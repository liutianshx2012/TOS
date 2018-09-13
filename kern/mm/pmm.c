/*************************************************************************
	> File Name: pmm.c
	> Author:TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 11时50分42秒
 ************************************************************************/
#include <defs.h>
#include <x86.h>
#include <mmu.h>
#include <sync.h>
#include <error.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <memlayout.h>
#include <default_pmm.h>
#include <pmm.h>
/* *
 * 全局 任务状态段ts
 * Task State Segment:
 *
 * The TSS may reside anywhere in memory. A special segment register called
 * the Task Register (TR) holds a segment selector that points a valid TSS
 * segment descriptor which resides in the GDT. Therefore, to use a TSS
 * the following must be done in function gdt_init:
 *   - create a TSS descriptor entry in GDT
 *   - add enough information to the TSS in memory as needed
 *   - load the TR register with a segment selector for that segment
 *
 * There are several fileds in TSS for specifying the new stack pointer when a
 * privilege level change happens. But only the fields SS0 and ESP0 are useful
 * in our os kernel.
 *
 * The field SS0 contains the stack segment selector for CPL = 0, and the ESP0
 * contains the new ESP value for CPL = 0. When an interrupt happens in protected
 * mode, the x86 CPU will look in the TSS for SS0 and ESP0 and load their value
 * into SS and ESP respectively.
 * */
static struct taskstate ts = {0};

/* virtual address of physical page array */
struct Page *pages;

/* amount of physical memory (int pages) */
size_t npage = 0;

/* virtual address of boot-time page directory */ 
pde_t *boot_pgdir = NULL;

/*physical address of boot-time page directory*/
//页式管理通过一个二级页表实现,CR3 寄存器存储着 一级页表的起始 physical addr, 4byte对齐,low 12bit=0 , 页表是驻留在内存中的表, 存放在物理地址空间中. 可以看做 2^20的物理地址数组.
uintptr_t boot_cr3;
/* physical memory management*/
const struct pmm_manager *pmm_manager;

/* *
 * The page directory entry corresponding to the virtual address range
 * [VPT, VPT + PT_SIZE) [0xFAC00000, 0xFAC00000 + 0x400000)points to the page directory *
 * itself. Thus, the page directory is treated as a page table as well as a page directory.
 * 
 * One result of treating the page directory as a page table is that all PTEs
 * can be accessed though a "virtual page table" at virtual address VPT. And the
 * PTE for number n is stored in vpt[n].
 *
 * A second consequence is that the contents of the current page directory will
 * always available at virtual address PG_ADDR(PDX(VPT), PDX(VPT), 0), to which
 * vpd is set bellow.
 * */
pte_t * const vpt = (pte_t *)VPT;
// vpd 的值就是 PDT 的起始 virtual addr 0xFAFEB000 (高10bit = 中10bit = 1003),确保vpt 是PDT 中
// 第一个 PDE 指向的 PT 的起始 virtual addr. 此时描述 kernel 虚拟空间的 PDT 的virtual addr 
//  = 0xFAFEB000, 大小=4kb. PT 的理论连续 virtual addr 空间为 0xFAC00000 ~ 0xFB000000,大小为4MB
// 可以有 1M 个 PTE, 即可映射 4GB 的地址空间 (实际上不会用完这么多页).
pde_t * const vpd = (pde_t *)PG_ADDR(PDX(VPT), PDX(VPT), 0);
/* *
 * Global Descriptor Table:
 * 全局描述表 gdt[]
 * The kernel and user segments are identical (except for the DPL). To load
 * the %ss register, the CPL must equal the DPL. Thus, we must duplicate the
 * segments for the user and the kernel. Defined as follows:
 *   - 0x0 :  unused (always faults -- for trapping NULL far pointers)
 *   - 0x8 :  kernel code segment
 *   - 0x10:  kernel data segment
 *   - 0x18:  user code segment
 *   - 0x20:  user data segment
 *   - 0x28:  defined for tss, initialized in gdt_init
 * */
/* six segment */
static struct segdesc gdt[] =
{
    SEG_NULL, /* first  segment descritor must be SEG_NULL */
    [SEG_K_TEXT] = SEG(STA_X | STA_R, 0x0, 0xFFFFffff, DPL_KERNEL),
    [SEG_K_DATA] = SEG(STA_W, 0x0, 0xffffFFFF, DPL_KERNEL),
    [SEG_U_TEXT] = SEG(STA_X | STA_R, 0x0, 0xffffFFFF, DPL_USER),
    [SEG_U_DATA] = SEG(STA_W, 0x0, 0xFFFFffff, DPL_USER),
    [SEG_K_TSS]  = SEG_NULL,
};

static struct pseudodesc gdt_pd =
{
    sizeof(gdt) - 1, (uintptr_t)gdt
};

static void check_alloc_page(void);
static void check_pgdir(void);
static void check_boot_pgdir(void);
/* *
 * lgdt - load the global descriptor table register and reset the
 * data/code segement registers for kernel.
 * */
/* 加载全局描述符寄存器的函数lgdt */
static inline void
lgdt(struct pseudodesc *pd)
{
    __asm__ volatile ("lgdt (%0)" :: "r" (pd));
    __asm__ volatile ("movw %%ax, %%gs" :: "a" (USER_DS));
    __asm__ volatile ("movw %%ax, %%fs" :: "a" (USER_DS));
    __asm__ volatile ("movw %%ax, %%es" :: "a" (KERNEL_DS));
    __asm__ volatile ("movw %%ax, %%ds" :: "a" (KERNEL_DS));
    __asm__ volatile ("movw %%ax, %%ss" :: "a" (KERNEL_DS));
    // reload cs
    __asm__ volatile ("ljmp %0, $1f\n 1:\n" :: "i" (KERNEL_CS));
}

/* *
 * load_esp0 - change the ESP0 in default task state segment,
 * so that we can use different kernel stack when we trap frame
 * user to kernel.
 * */
void
load_esp0(uintptr_t esp0)
{
    ts.ts_esp0 = esp0;
}

/* gdt_init - initialize the default GDT and TSS*/
static void
gdt_init(void)
{
    load_esp0((uintptr_t)bootstacktop);
    ts.ts_ss0 = KERNEL_DS; // 0x10

    /*initialize the TSS filed of the gdt*/
    gdt[SEG_K_TSS] = SEG_TSS(SST_T32A,(uintptr_t)&ts,sizeof(ts),DPL_KERNEL);

    /*reload all Segment registers*/
    lgdt(&gdt_pd);
    /*load the TSS*/
    ltr(G_D_TSS);
}

/* init_pmm_manager - initialize a pmm_manager instance */
static void
init_pmm_manager(void)
{
    pmm_manager = &default_pmm_manager;
    cprintf("memory management: %s\n", pmm_manager->name);
    pmm_manager->init();
}

//init_memmap - call pmm->init_memmap to build Page struct for free memory
// Page->flags = 0; && Page->ref = 0; 并且添加到 free_area.free_list 指向的list中,
// 为将来的空闲页面管理做好初始化工作.
static void
init_memmap(struct Page *base, size_t n)
{
    pmm_manager->init_memmap(base, n);
}
// call pmm->alloc_pages to allocate a continuous  n * PAGESIZE memory
struct Page *
alloc_pages(size_t n)
{
    struct Page *page = NULL;
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        page = pmm_manager->alloc_pages(n);
    }
    local_intr_restore(intr_flag);
    return page;
}


//free_pages - call pmm->free_pages to free a continuous n*PAGESIZE memory
void
free_pages(struct Page *base, size_t n)
{
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        pmm_manager->free_pages(base, n);
    }
    local_intr_restore(intr_flag);
}

//nr_free_pages - call pmm->nr_free_pages to get the size (nr*PAGESIZE)
//of current free memory
size_t
nr_free_pages(void)
{
    size_t ret;
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        ret = pmm_manager->nr_free_pages();
    }
    local_intr_restore(intr_flag);
    return ret;
}
/*
e820map:
  memory: 0009fc00, [00000000, 0009fbff], type = 1.
  memory: 00000400, [0009fc00, 0009ffff], type = 2.
  memory: 00010000, [000f0000, 000fffff], type = 2.
  memory: 07ee0000, [00100000, 07fdffff], type = 1.
  memory: 00020000, [07fe0000, 07ffffff], type = 2.
  memory: 00040000, [fffc0000, ffffffff], type = 2.

  type =1 才能被 kernel 使用, [0x0009fc00] = 639kb 被 struct page 占用
           所以 kernel 只能从 [0x00100000] 开始使用空闲的 phys addr.
           
           0x07fdffff - 0x07fe0000 = 0x7E27000 B 这么大的 空闲 phys addr 
           能分配 0x7E27000 / 4096 = 32295 个 free struct page frame (全部未标记))
           0x001b9000 - 0x00100000 = 0xB9000 = 740 KB

*/
static void
page_init(void)
{
    struct e820map *memmap = (struct e820map*)(0x8000 + KERN_BASE);
    uint64_t maxpa = 0; //max physical address
    cprintf("e820map:\n");
    int i;
    for (int i=0;i<memmap->nr_map;i++) {
        uint64_t begin = memmap->map[i].addr;
        uint64_t end = begin + memmap->map[i].size;
        cprintf("  memory: %08llx, [%08llx, %08llx], type = %d.\n",
                memmap->map[i].size, begin, end - 1, memmap->map[i].type);
        if (memmap->map[i].type == E820_KERN) {
            if (maxpa < end && begin < KMEMSIZE) {
                maxpa = end;
            }
        }
    }
    if (maxpa > KMEMSIZE) {
        maxpa = KMEMSIZE;
    }

    extern char end[]; /* kernel.ld 定义 bootloader 加载 kernel elf 的结束链接地址*/
    npage = maxpa / PAGE_SIZE;
    // cprintf("\n\n max phys page frame numbers [%d]\n",npage);
    /* 管理页级物理内存空间所需的 struct Page 共占用的 内存的起始地址*/
    pages = (struct Page *)ROUNDUP((void*)end, PAGE_SIZE);
    
    cprintf("elf kernel end addr [0x%08lx] [0x%08lx] manager mem size [%d]\n",PADDR((uintptr_t)pages),pages,npage * sizeof(struct Page));

    for (i=0;i<npage;i++) {
        SetPageReserved(pages + i);
    }
    /* 0 ~ pages + sizeof(struct Page) * npage) 之间的物理内存空间设定为 已占用的物理内存(存储了 npage 个 struct Page )  起始 0 ~~~ 640KB 是空闲的*/
    // freemem 是这时空闲空间的起始地址. 上面的 SetPageReserved 把两部分空间给标识出来,标识已占用
    uintptr_t freemem = PADDR((uintptr_t)pages + sizeof(struct Page) * npage);//0x001b8d80  = 1763kb
    cprintf("freemem phys addr [0x%08lx]\n",freemem);

    for (i=0;i<memmap->nr_map;i++) {
        uint64_t begin = memmap->map[i].addr;
        uint64_t end = begin + memmap->map[i].size;
        if (memmap->map[i].type == E820_KERN) {
            if (begin < freemem) {
                begin = freemem;
            }
            if (end > KMEMSIZE) {
                end = KMEMSIZE;
            }
            if (begin < end) {
                begin = ROUNDUP(begin, PAGE_SIZE);
                end = ROUNDDOWN(end, PAGE_SIZE);
                if (begin < end) {
                    // [001b9000, 07fe0000]
                    cprintf("begin~end [%08llx, %08llx]\n",begin,end);
                    // 根据探测到的空闲物理空间,设置空闲标记,获得空闲空间的起始地址和结束地址
                    init_memmap(pa2page(begin), (end - begin) / PAGE_SIZE);
                }
            }
        }
    }
}

static void
check_alloc_page(void)
{
    pmm_manager->check();
    cprintf("check_alloc_page() succeeded!\n");
}

//boot_alloc_page - allocate one page using pmm->alloc_pages(1)
// return value: the kernel virtual address of this allocated page
//note: this function is used to get the memory for PDT(Page Directory Table) & PT(Page Table)
static void *
boot_alloc_page(void)
{
    struct Page *p = alloc_page();
    if (p == NULL) {
        panic("boot_alloc_page failed.\n");
    }
    return page2kva(p);
}

//get_pte - get pte and return the kernel virtual address of this pte for la
//        - if the PT contians this pte didn't exist, alloc a page for PT
// parameter:
//  pgdir:  the kernel virtual base address of PDT
//  la:     the linear address need to map
//  create: a logical value to decide if alloc a page for PT
// return vaule: the kernel virtual address of this pte
// 内存管理经常需要查页表:此函数找到一个 virtual addr 对应的 二级页表项(PTE)的 内核虚地址,如果此二级页表项不存在,则分配一个包含此项的二级页表.
/*
   boot_pgdir  virtual addr =  [0xc01b9000] 指向 PDT 本身的虚拟起始地址
   boot_cr3    physical addr = [0x001b9000] 指向 PDT 本身的物理起始地址
   boot_pgdir - boot_cr3 = [0x0c000000]  映射关系
 
   pdep pointer = [0x001ba007] 指向 PDT 中第一个 PDE 的指针 liner addr
   first pde phys addr = [0x001ba000] PDT 中第一个 PDE 的物理起始地址    PDE_ADDR(*pdep)
   first pde vitr addr = [0xc01ba000] PDT 中第一个 PDE 的虚拟起始地址 KADDR(PDE_ADDR(*pdep))
   first pde vitr addr - first pde phys addr = [0x0c000000]  映射关系

   page table index  = PTX(la)

   pte phys addr = &((pte_t *)(PDE_ADDR(*pdep)))[PTX(la)]
   pte virt addr = &((pte_t *)KADDR(PDE_ADDR(*pdep)))[PTX(la)]

   first la ==>[c0000000]
   first pte vitr addr in first pde = [0xc01ba000]
   first pte phys addr in first pde = [0x001ba000]
  
   first pte phys addr in first pde - first pte vitr addr in first pde = [0x0c000000]  映射关系

   second la ==>[c0001000]
   second pte vitr addr in first pde = [0xc01ba004]
   second pte phys addr in first pde = [0x001ba004]

   second pte phys addr in first pde - second pte vitr addr in first pde = [0x0c000000]  映射关系
*/
pte_t *
get_pte(pde_t *pgdir, uintptr_t la, bool create)
{
    // (1) find page directory entry 尝试获取页表 
    pde_t *pdep = &pgdir[PDX(la)]; 
    // (2) check if entry is not present 若获取不成功则申请一页;
    if (!(*pdep & PTE_P)) {  
        struct Page *page;
        // (3) check if creating is needed, then alloc page for page table
        //如果create参数为0,则get_pte返回NULL;如果create参数不为0,则get_pte需要申请一个新的物理页（通过alloc_page来实现
        if (!create || (page = alloc_page()) == NULL) {
            return NULL;
        }
        // (4) set page reference 引用次数需要加1 
        set_page_ref(page, 1);
        // (5) get linear address of page 获取页的线性地址 
        uintptr_t pa = page2pa(page);
        // (6) clear page content using memset 新申请的 page 清 0
        memset(KADDR(pa), 0, PAGE_SIZE); 
        // (7) set page directory entry's permission 设置权限  
        *pdep = pa | PTE_U | PTE_W | PTE_P;
    }

    // cprintf("pte idx =>%d Phy PDE_ADDR ==>[%08lx] Vir PDE_ADDR ==>[%08lx]\n\t Phy PTE_ADDR ==>[%08lx] Vir PTE_addr ==>[%08lx] la ==>[%08lx]  pgdir==>[%08lx]\n",PTX(la),PDE_ADDR(*pdep),KADDR(PDE_ADDR(*pdep)),&((pte_t *)(PDE_ADDR(*pdep)))[PTX(la)],&((pte_t *)KADDR(PDE_ADDR(*pdep)))[PTX(la)],la, pgdir);

    // (8) return page table entry 返回页表 virtual addr 
    return &((pte_t *)KADDR(PDE_ADDR(*pdep)))[PTX(la)]; 
}
//get_page - get related Page struct for linear address la using PDT pgdir
struct Page *
get_page(pde_t *pgdir, uintptr_t la, pte_t **ptep_store)
{
    pte_t *ptep = get_pte(pgdir, la, 0);
    if (ptep_store != NULL) {
        *ptep_store = ptep;
    }
    if (ptep != NULL && *ptep & PTE_P) {
        return pte2page(*ptep);
    }
    return NULL;
}

//page_remove_pte - free an Page sturct which is related linear address la
//                - and clean(invalidate) pte which is related linear address la
//note: PT is changed, so the TLB need to be invalidate
/*释放某虚拟地址所在的页并取消对应的二级页表项的映射*/
static inline void
page_remove_pte(pde_t *pgdir, uintptr_t la, pte_t *ptep)
{
    //(1) check if page directory is present 判断页表中该表项是否存在
    if (*ptep & PTE_P) {   	
        //(2) find corresponding page to pte		  
        struct Page *page = pte2page(*ptep);
        //(3) decrease page reference 判断是否只被引用了一次
        if (page_ref_dec(page) == 0) {
           // (4) and free this page when page reference reachs 0 如果只被引用了一次,那么可以释放掉此页
            free_page(page); 	      
        }
        // (5) clear second page table entry 如果被多次引用,则不能释放此页,只用释放二级页表的表项
        *ptep = 0;
        //(6) flush tlb	 更新页表				  
        tlb_invalidate(pgdir, la);   
    }
}

//page_remove - free an Page which is related linear address la and has an validated pte
void
page_remove(pde_t *pgdir, uintptr_t la)
{
    pte_t *ptep = get_pte(pgdir, la, 0);
    if (ptep != NULL) {
        page_remove_pte(pgdir, la, ptep);
    }
}

// invalidate a TLB entry, but only if the page tables being
// edited are the ones currently in use by the processor.
// 取消 la 对应物理页之间的映射关系,相当于 flush TLB,每次调整虚拟页和物理页之间的映射关系的时候,
// 都需要 flush TLB.  TLB (translation Lookaside Buffer) 来缓存 linear addr 到 physical addr 
// 的映射关系. 实际的地址转换过程中, CPU 首先根据 linear addr 来查找 TLB ,如果未发现该 linear addr 
// 到 physical addr 的映射关系( TLB miss),将根据页表中的映射关系填充 TLB (TLB fill),然后再进行转换
void
tlb_invalidate(pde_t *pgdir, uintptr_t la)
{
    if (rcr3() == PADDR(pgdir)) {
        invlpg((void *)la);
    }
}

//page_insert - build the map of phy addr of an Page with the linear addr la
// paramemters:
//  pgdir: the kernel virtual base address of PDT
//  page:  the Page which need to map
//  la:    the linear address need to map
//  perm:  the permission of this Page which is setted in related pte
// return value: always 0
//note: PT is changed, so the TLB need to be invalidate
int
page_insert(pde_t *pgdir, struct Page *page, uintptr_t la, uint32_t perm)
{
    pte_t *ptep = get_pte(pgdir, la, 1);
    if (ptep == NULL) {
        return -E_NO_MEM;
    }
    page_ref_inc(page);
    if (*ptep & PTE_P) {
        struct Page *p = pte2page(*ptep);
        if (p == page) {
            page_ref_dec(page);
        } else {
            page_remove_pte(pgdir, la, ptep);
        }
    }
    *ptep = page2pa(page) | PTE_P | perm;
    tlb_invalidate(pgdir, la);
    return 0;
}

static void
check_pgdir(void)
{
    assert(npage <= KMEMSIZE / PAGE_SIZE);
    assert(boot_pgdir != NULL && (uint32_t)PG_OFF(boot_pgdir) == 0);
    assert(get_page(boot_pgdir, 0x0, NULL) == NULL);

    struct Page *p1, *p2;
    p1 = alloc_page();
    // cprintf("p1 ==>[%08lx]\n",p1);
    assert(page_insert(boot_pgdir, p1, 0x0, 0) == 0);

    pte_t *ptep;
    assert((ptep = get_pte(boot_pgdir, 0x0, 0)) != NULL);
    assert(pte2page(*ptep) == p1);
    assert(page_ref(p1) == 1);

    ptep = &((pte_t *)KADDR(PDE_ADDR(boot_pgdir[0])))[1];
    assert(get_pte(boot_pgdir, PAGE_SIZE, 0) == ptep);

    p2 = alloc_page();
    assert(page_insert(boot_pgdir, p2, PAGE_SIZE, PTE_U | PTE_W) == 0);
    assert((ptep = get_pte(boot_pgdir, PAGE_SIZE, 0)) != NULL);
    assert(*ptep & PTE_U);
    assert(*ptep & PTE_W);
    assert(boot_pgdir[0] & PTE_U);
    assert(page_ref(p2) == 1);

    assert(page_insert(boot_pgdir, p1, PAGE_SIZE, 0) == 0);
    assert(page_ref(p1) == 2);
    assert(page_ref(p2) == 0);
    assert((ptep = get_pte(boot_pgdir, PAGE_SIZE, 0)) != NULL);
    assert(pte2page(*ptep) == p1);
    assert((*ptep & PTE_U) == 0);

    page_remove(boot_pgdir, 0x0);
    assert(page_ref(p1) == 1);
    assert(page_ref(p2) == 0);

    page_remove(boot_pgdir, PAGE_SIZE);
    assert(page_ref(p1) == 0);
    assert(page_ref(p2) == 0);

    assert(page_ref(pde2page(boot_pgdir[0])) == 1);
    free_page(pde2page(boot_pgdir[0]));
    boot_pgdir[0] = 0;

    cprintf("check_pgdir() succeeded!\n");
}

static void
enable_paging(void)
{
    lcr3(boot_cr3);

    // turn on paging   PG = 1
    uint32_t cr0 = rcr0();
    cr0 |= CR0_PE | CR0_PG | CR0_AM | CR0_WP | CR0_NE | CR0_TS | CR0_EM | CR0_MP;
    cr0 &= ~(CR0_TS | CR0_EM);
    lcr0(cr0);
}

//boot_map_segment - setup & enable the paging mechanism parameters
//  la:   linear address of this memory need to map (after x86 segment map)
//  size: memory size
//  pa:   physical address of this memory
//  perm: permission of this memory
static void
boot_map_segment(pde_t *pgdir, uintptr_t la, size_t size, uintptr_t pa, uint32_t perm)
{
    assert(PG_OFF(la) == PG_OFF(pa));
    size_t n = ROUNDUP(size + PG_OFF(la), PAGE_SIZE) / PAGE_SIZE;// n => 229376
    la = ROUNDDOWN(la, PAGE_SIZE);
    pa = ROUNDDOWN(pa, PAGE_SIZE);
    // cprintf("page frame nums =>%d la ==>[%08lx] pa ==>[%08lx] pgdir==>[%08lx]\n",n,la, pa,pgdir);

    for (; n > 0; n --, la += PAGE_SIZE, pa += PAGE_SIZE) {
        pte_t *ptep = get_pte(pgdir, la, 1);
        assert(ptep != NULL);
        *ptep = pa | PTE_P | perm;
    }
}

static void
check_boot_pgdir(void)
{
    pte_t *ptep;
    int i;
    for (i = 0; i < npage; i += PAGE_SIZE) {
        assert((ptep = get_pte(boot_pgdir, (uintptr_t)KADDR(i), 0)) != NULL);
        assert(PTE_ADDR(*ptep) == i);
    }

    assert(PDE_ADDR(boot_pgdir[PDX(VPT)]) == PADDR(boot_pgdir));

    assert(boot_pgdir[0] == 0);

    struct Page *p;
    p = alloc_page();
    assert(page_insert(boot_pgdir, p, 0x100, PTE_W) == 0);
    assert(page_ref(p) == 1);
    assert(page_insert(boot_pgdir, p, 0x100 + PAGE_SIZE, PTE_W) == 0);
    assert(page_ref(p) == 2);

    const char *str = "tos: Hello world!!";
    strcpy((void *)0x100, str);
    assert(strcmp((void *)0x100, (void *)(0x100 + PAGE_SIZE)) == 0);

    *(char *)(page2kva(p) + 0x100) = '\0';
    assert(strlen((const char *)0x100) == 0);

    free_page(p);
    free_page(pde2page(boot_pgdir[0]));
    boot_pgdir[0] = 0;

    cprintf("check_boot_pgdir() succeeded!\n");
}
/* pmm_init - setup a pmm to manage physical memory, build  PDT & PT to setup paging
 * mechanism  check the correctness of pmm & paging mechanism , print PDT & PT
 **/
void
pmm_init(void)
{   
    /* We need to alloc | free the physical memory (granularity is 4KB or other size).
     * So a framework of physical memory manager (struct pmm_manger) is defined in pmm.h
     * First  we  should init  a physical memory manager(pmm) based on the  framework.
     * Then  pmm can alloc | free the physical  memory.
     * Now the first_fit | best_fit | worst_fit | buddy_system pmm are available. 
     */
    // step 1
    init_pmm_manager(); 
    // step 2,建立空闲的 page list
    // detect physical memory space, reserve already used memory,
    // then use pmm->init_memmap to create free page list
    page_init();
    // step 3--> 检查物理内存分配算法
    //use pmm->check to verify the correctness of the alloc/free function in a pmm
    check_alloc_page();
    // step 4--> 建立一个临时二级页表 PT, 确保切换到分页机制后,代码能够正常执行.
    // create boot_pgdir, an initial page directory(Page Directory Table, PDT)
    boot_pgdir = boot_alloc_page();
    memset(boot_pgdir, 0, PAGE_SIZE);//4kb
    // cprintf("boot_pgdir  virtual addr ==>[%08lx]\n",boot_pgdir);//c01b9000
    boot_cr3 = PADDR(boot_pgdir);
    // cprintf("boot_cr3 physical addr ==>[%08lx]\n",boot_cr3);//001b9000
    // c01b9000(virtual addr) - 001b9000(physical addr)  = 0xC0000000 stage 2 映射关系

    check_pgdir();

    static_assert(KERN_BASE % PT_SIZE == 0 && KERN_TOP % PT_SIZE == 0);


    // recursively insert boot_pgdir in itself
    // to form a virtual page table at virtual address VPT
    boot_pgdir[PDX(VPT)] = PADDR(boot_pgdir) | PTE_P | PTE_W;//PDX(VPT) = 1003
    // step 5--> 建立一一映射关系的二级页表
    // map all physical memory to linear memory with base linear addr KERN_BASE
    //linear_addr KERN_BASE~KERN_BASE+KMEMSIZE = phy_addr 0~KMEMSIZE
    //But shouldn't use this map until enable_paging() & gdt_init() finished.
    boot_map_segment(boot_pgdir, KERN_BASE, KMEMSIZE, 0, PTE_W);

    //temporary map:
    //virtual_addr 3G~3G+4M = linear_addr 0~4M = linear_addr 3G~3G+4M = phy_addr 0~4M
    // cprintf("page directory index ==>%d \n",PDX(KERN_BASE));
    boot_pgdir[0] = boot_pgdir[PDX(KERN_BASE)]; //PDX(KERN_BASE) = 768
    // step 6--> 使能分页机制
    enable_paging();
    //step 7 --> 重新设置全局段描述表 GDT
    //reload gdt(third time,the last time) to map all physical memory
    //virtual_addr 0~4G=liear_addr 0~4G
    //then set kernel stack(ss:esp) in TSS, setup TSS in gdt, load TSS
    gdt_init();
    //step 8 -->取消临时二级页表
    //disable the map of virtual_addr 0~4M
    boot_pgdir[0] = 0;
    // step 9--> 检查页表建立是否正确
    //now the basic virtual memory map(see memalyout.h) is established.
    //check the correctness of the basic virtual memory map.
    check_boot_pgdir();
    //step 10-->通过自映射机制完成页表的打印输出
    print_pgdir();
}


//perm2str - use string 'u,r,w,-' to present the permission
static const char *
perm2str(int perm)
{
    static char str[4];
    str[0] = (perm & PTE_U) ? 'u' : '-';
    str[1] = 'r';
    str[2] = (perm & PTE_W) ? 'w' : '-';
    str[3] = '\0';
    return str;
}

//get_pgtable_items - In [left, right] range of PDT or PT, find a continuous linear addr space
//                  - (left_store*X_SIZE ~ right_store*X_SIZE) for PDT or PT
//                  - X_SIZE=PT_SIZE=4M, if PDT; X_SIZE=PAGE_SIZE=4K, if PT
// paramemters:
//  left:        no use ???
//  right:       the high side of table's range
//  start:       the low side of table's range
//  table:       the beginning addr of table
//  left_store:  the pointer of the high side of table's next range
//  right_store: the pointer of the low side of table's next range
// return value: 0 - not a invalid item range, perm - a valid item range with perm permission
static int
get_pgtable_items(size_t left, size_t right, size_t start, uintptr_t *table, size_t *left_store, size_t *right_store)
{
    if (start >= right) {
        return 0;
    }
    while (start < right && !(table[start] & PTE_P)) {
        start ++;
    }
    if (start < right) {
        if (left_store != NULL) {
            *left_store = start;
        }
        int perm = (table[start ++] & PTE_USER);
        while (start < right && (table[start] & PTE_USER) == perm) {
            start ++;
        }
        if (right_store != NULL) {
            *right_store = start;
        }
        return perm;
    }
    return 0;
}

//print_pgdir - print the PDT & PT
// 页表自映射方式完成对整个 PDT & PT 的内容扫描和打印. 
// waring: 这里不会出现某个 PT 的 virtual addr 与 PDT 的 virtual addr 相同的情况.
// 类似于 qemu 的 info pg 相同的功能, 即 能够从内存中,将当前页表内有效数据 PTE_P 打印出来.
/*
-------------------- BEGIN --------------------
PDE(0e0) c0000000-f8000000 38000000 urw
  |-- PTE(38000) c0000000-f8000000 38000000 -rw
PDE(001) fac00000-fb000000 00400000 -rw
  |-- PTE(000e0) faf00000-fafe0000 000e0000 urw
  |-- PTE(00001) fafeb000-fafec000 00001000 -rw
--------------------- END ---------------------
 
 PDE(0e0)==> 0x0e0 表示 PDE 表中相邻的 224 项具有相同的权限;
     0xc0000000-0xf8000000 表示 PDE 表中,这相邻的两项所映射的线性地址范围;
     0x38000000 同意表示范围, 0xf8000000 - 0xc0000000 = 0x38000000;
     urw => PDE 表中所给出的权限位, u==>用户可读(PTE_U),  r==> PTE_P , w==>PTE_W 用户可写

PDE(001)==> 0x001表示 仅 1 条连续的 PDE 表项具备相同的属性. 相应的,在这条表项中遍历找到 2 组 PTE 表项

warning: 1==> PTE 中输出的权限是 PTE 表中的数据给出的,并没有和 PDE 表中权限做运算.
         2==> 整个 print_pgdir 函数强调两点:  第一是相同权限,第二是连续.
         3==> print_pgdir 中用到了 vpt & vpd 两个变量.

自映射机制还可方便用户态程序访问页表.  因为页表是kernel 维护, user application 很难知道自己页表的映射结
构. VPT 实际上是在 kernel addr zone. 我们可以用同意的方式实现一个 user addr zone 的映射结构(eg==>
pgdir[UVPT] = PADDR(pgdir) | PTE_P | PTE_U), 注意这里不能给 PTE_W 权限,并且 pgdir 是每个进程的 page table, 而不是 boot_pgdir ), 这样, user applications 就可以用和 kernel 一样的 print_pgdir函数遍历自己的 页表结构了.
*/
void
print_pgdir(void)
{
    cprintf("-------------------- BEGIN --------------------\n");
    size_t left, right = 0, perm;
    while ((perm = get_pgtable_items(0, N_PDE_ENTRY, right, vpd, &left, &right)) != 0) {
        cprintf("PDE(%03x) %08x-%08x %08x %s\n", right - left,
                left * PT_SIZE, right * PT_SIZE, (right - left) * PT_SIZE, perm2str(perm));
        size_t l, r = left * N_PTE_ENTRY;
        while ((perm = get_pgtable_items(left * N_PTE_ENTRY, right * N_PTE_ENTRY, r, vpt, &l, &r)) != 0) {
            cprintf("  |-- PTE(%05x) %08x-%08x %08x %s\n", r - l,
                    l * PAGE_SIZE, r * PAGE_SIZE, (r - l) * PAGE_SIZE, perm2str(perm));
        }
    }
    cprintf("--------------------- END ---------------------\n");
}
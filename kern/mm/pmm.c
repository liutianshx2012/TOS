/*************************************************************************
	> File Name: pmm.c
	> Author:TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 11时50分42秒
 ************************************************************************/
#include <defs.h>
#include <x86.h>
#include <mmu.h>
#include <memlayout.h>
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
    sizeof(gdt) - 1, (uint32_t)gdt
};

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

/* temporary kernel stack*/
uint8_t stack0[1024]; 

/* gdt_init - initialize the default GDT and TSS*/
/* 只有在保护模式下才能使用分段存储管理机制 ;
 * 分段机制将内存划分成以起始地址和长度限制这两个二维参数表示的内存块,这些内存块就称之为段 (Segment).
 * 编译器把源程序编译成执行程序时用到的 代码段、数据段、堆和栈等概念在这里可以与段联系起来,
 * 二者在含义上是一致的
 **/
/* 对全局描述符表和任务状态段的初始化函数 gdt_init */
static void
gdt_init(void)
{
    /* setup a TSS so that we can get the right stack when we trap from
     * user to the kernel.But not safe here,it's only a temporary value,
     * it will be set to KSTACKTOP in proj2.*/
    /*
       &stack0  = 0x10f980
       ts.ts_esp0 = 0x10f980 + (0x400)1024 = 0x10fd80
    */
    ts.ts_esp0 =(uint32_t)&stack0 + sizeof(stack0);
    ts.ts_ss0 = KERNEL_DS; // 0x10

    /*initialize the TSS filed of the gdt*/
    gdt[SEG_K_TSS] = SEG_TSS(SST_T32A,(uint32_t)&ts,sizeof(ts),DPL_KERNEL);
    gdt[SEG_K_TSS].sd_s = 0;

    /*reload all Segment registers*/
    lgdt(&gdt_pd);
    /*load the TSS*/
    ltr(G_D_TSS);
}

/* pmm_init - initialize the physical memory management*/
/* GDT (Global Descriptor Table ) 和LDT (Local Descriptor Table),每一张段表可以包含
 *  8192(2^13)个描述符,因而最多可以同时 存在2 * 2^13 = 2 ^14个段.
 *  虽然保护模式下可以有这么多段,逻辑地址空间看起来很大,但实际上段并不能扩展物理地址空间,
 *  很大程度上各个段的地址空间是相互重叠的.目前所谓的64TB(2^(14+32) = 2 ^46)逻辑地址空间是
 *  一个理论值,没有实际意义.在32bit保护模式下,真正的物理空间仍然只有2^32字节那么大.
 **/
void
pmm_init(void)
{
    gdt_init();
}

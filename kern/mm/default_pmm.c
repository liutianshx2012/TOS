/*************************************************************************
	> File Name: default_pmm.c
	> Author:TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年07月01日 星期五 11时55分34秒
 ************************************************************************/
#include <pmm.h>
#include <list.h>
#include <string.h>
#include <default_pmm.h>

/* In the first fit algorithm, the allocator keeps a list of free blocks (known as the free list) and,
   on receiving a request for memory, scans along the list for the first block that is large enough to
   satisfy the request. If the chosen block is significantly larger than that requested, then it is
   usually split, and the remainder added to the list as another free block.
*/
free_area_t free_area;

#define free_list (free_area.free_list)
#define nr_free (free_area.nr_free)

static void
default_init(void)
{
    list_init(&free_list);
    nr_free = 0;
}
/*
 base : 某个连续地址的空闲块的起始页
 n  : 页个数
 根据每个 phys page frame 的情况来建立空闲页链表,且空闲页块应该是根据 地址高低形成一个有序链表.
*/
// kern_init --> pmm_init --> init_memmap --> pmm_manager->init
static void
default_init_memmap(struct Page *base, size_t n)
{
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(PageReserved(p)); //确认本页是否为保留页
        p->flags = 0; 	//设置标志位
        SetPageProperty(p);
        p->property = 0;
        set_page_ref(p, 0); //清空引用
        list_add_before(&free_list, &(p->page_link));//插入空闲页的链表里面
    }
    nr_free += n;//说明连续有n个空闲块,属于空闲链表
    //first block
    base->property = n;//连续内存空闲块的大小为n,属于物理页管理链表
}
/* 
 * n : 需要分配 n 个 page.
 * 从空闲页块的链表中去遍历,找到第一块大小大于n的块,然后分配出来,把它从空闲页链表中除去,
 * 然后如果有多余的,把分完剩下的部分再次加入会空闲页链表中即可.
 * */
static struct Page *
default_alloc_pages(size_t n)
{
    assert(n > 0);
    //如果所有的空闲页的加起来的大小都不够,那直接返回NULL
    if (n > nr_free) {  
        return NULL;
    }
    list_entry_t *le = &free_list; //从空闲块链表的头指针开始
    list_entry_t *next_le;
    //依次往下寻找直到回到头指针处,即已经遍历一次
    while((le=list_next(le)) != &free_list) { 
        //将地址转换成页的结构
        struct Page *p = le2page(le, page_link); 
        //由于是first-fit,则遇到的第一个大于等于N的块就选中即可
        if(p->property >= n) {  
            int i;
            // 找到 first fit block 后,就要从新组织空闲块,然后把找到的 page 返回.
            for(i=0; i<n; i++) {
                next_le = list_next(le); //下一个空闲块
                struct Page *pp = le2page(le, page_link);//下一个空闲块对应的 page
                SetPageReserved(pp);
                ClearPageProperty(pp);
                list_del(le);
                le = next_le;
            }
            //如果分配的多了需要剔除
            if(p->property>n) {
                //如果选中的第一个连续的块大于n,只取其中的大小为n的块
                (le2page(le,page_link))->property = p->property - n;
            }
            ClearPageProperty(p);
            SetPageReserved(p);
            //当前空闲页的数目减n
            nr_free -= n;  
            return p;
        }
    }
    //没有大于等于n的连续空闲页块,返回空
    return NULL;
}
/*
  default_alloc_pages 的逆过程,另外需要考虑空闲块的合并问题.
  释放已经使用完的页,把他们合并到free_list中
*/
static void
default_free_pages(struct Page *base, size_t n)
{
    assert(n > 0);
    assert(PageReserved(base));
    //查找该插入的位置le
    list_entry_t *le = &free_list;
    struct Page * p;
    while((le=list_next(le)) != &free_list) {
        p = le2page(le, page_link);
        if(p>base) {
            break;
        }
    }
    //list_add_before(le, base->page_link);
    //向le之前插入n个页(空闲),并设置标志位
    for(p=base; p<base+n; p++) {
        list_add_before(le, &(p->page_link));
    }
    base->flags = 0;
    set_page_ref(base, 0);
    ClearPageProperty(base);
    SetPageProperty(base);
    //将页块信息记录在头部
    base->property = n; 
    //是否需要合并,向高地址合并
    p = le2page(le,page_link) ; 
    if( base+n == p ) {
        base->property += p->property;
        p->property = 0;
    }
    //向低地址合并
    le = list_prev(&(base->page_link));
    p = le2page(le, page_link);
    //若低地址已分配则不需要合并
    if(le!=&free_list && p==base-1) { 
        while(le!=&free_list) {
            if(p->property) {
                p->property += base->property;
                base->property = 0;
                break;
            }
            le = list_prev(le);
            p = le2page(le,page_link);
        }
    }

    nr_free += n;
    return ;
}

static size_t
default_nr_free_pages(void)
{
    return nr_free;
}

static void
basic_check(void)
{
    struct Page *p0, *p1, *p2;
    p0 = p1 = p2 = NULL;
    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);

    assert(p0 != p1 && p0 != p2 && p1 != p2);
    assert(page_ref(p0) == 0 && page_ref(p1) == 0 && page_ref(p2) == 0);

    assert(page2pa(p0) < npage * PAGE_SIZE);
    assert(page2pa(p1) < npage * PAGE_SIZE);
    assert(page2pa(p2) < npage * PAGE_SIZE);

    list_entry_t free_list_store = free_list;
    list_init(&free_list);
    assert(list_empty(&free_list));

    unsigned int nr_free_store = nr_free;
    nr_free = 0;

    assert(alloc_page() == NULL);

    free_page(p0);
    free_page(p1);
    free_page(p2);
    assert(nr_free == 3);

    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);

    assert(alloc_page() == NULL);

    free_page(p0);
    assert(!list_empty(&free_list));

    struct Page *p;
    assert((p = alloc_page()) == p0);
    assert(alloc_page() == NULL);

    assert(nr_free == 0);
    free_list = free_list_store;
    nr_free = nr_free_store;

    free_page(p);
    free_page(p1);
    free_page(p2);
}

// proj 2: below code is used to check the first fit allocation algorithm
static void
default_check(void)
{
    int count = 0, total = 0;
    list_entry_t *le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        assert(PageProperty(p));
        count ++, total += p->property;
    }
    assert(total == nr_free_pages());

    basic_check();

    struct Page *p0 = alloc_pages(5), *p1, *p2;
    assert(p0 != NULL);
    assert(!PageProperty(p0));

    list_entry_t free_list_store = free_list;
    list_init(&free_list);
    assert(list_empty(&free_list));
    assert(alloc_page() == NULL);

    unsigned int nr_free_store = nr_free;
    nr_free = 0;

    free_pages(p0 + 2, 3);
    assert(alloc_pages(4) == NULL);
    assert(PageProperty(p0 + 2) && p0[2].property == 3);
    assert((p1 = alloc_pages(3)) != NULL);
    assert(alloc_page() == NULL);
    assert(p0 + 2 == p1);

    p2 = p0 + 1;
    free_page(p0);
    free_pages(p1, 3);
    assert(PageProperty(p0) && p0->property == 1);
    assert(PageProperty(p1) && p1->property == 3);

    assert((p0 = alloc_page()) == p2 - 1);
    free_page(p0);
    assert((p0 = alloc_pages(2)) == p2 + 1);

    free_pages(p0, 2);
    free_page(p2);

    assert((p0 = alloc_pages(5)) != NULL);
    assert(alloc_page() == NULL);

    assert(nr_free == 0);
    nr_free = nr_free_store;

    free_list = free_list_store;
    free_pages(p0, 5);

    le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        count --, total -= p->property;
    }
    assert(count == 0);
    assert(total == 0);
}
/*物理内存页管理器,可以用它来获得所需的空闲物理页*/
const struct pmm_manager default_pmm_manager =
{
    .name = "default_pmm_manager",
    .init = default_init,
    .init_memmap = default_init_memmap,
    .alloc_pages = default_alloc_pages,
    .free_pages = default_free_pages,
    .nr_free_pages = default_nr_free_pages,
    .check = default_check,
};


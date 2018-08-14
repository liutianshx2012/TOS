/*************************************************************************
	> File Name: memlayout.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 12时58分47秒
 ************************************************************************/
#ifndef _KERN_MM_MEMLAYOUT_H
#define _KERN_MM_MEMLAYOUT_H

/* This file contains the definitions for memory management in our OS. */

/* global segment number */
#define SEG_K_TEXT 1
#define SEG_K_DATA 2
#define SEG_U_TEXT 3
#define SEG_U_DATA 4
#define SEG_K_TSS 5

/* global descriptor numbers */
#define G_D_K_TEXT ((SEG_K_TEXT) << 3) // kernel text   8(0x8)	=> (1000)
#define G_D_K_DATA ((SEG_K_DATA) << 3) // kernel data   16(0x10) => (1010)
#define G_D_U_TEXT ((SEG_U_TEXT) << 3) // user text     24(0x18) => (0001 1000)
#define G_D_U_DATA ((SEG_U_DATA) << 3) // user data  32(0x20) => (0010 0000)
#define G_D_TSS ((SEG_K_TSS) << 3)	 // task segment selector 40(0x28) => (0010 1000)

#define DPL_KERNEL (0) // 0x0 => (0000)
#define DPL_USER (3)   // 0x3 => (0011)
/* segment sector (not segment descriptor)*/
#define KERNEL_CS ((G_D_K_TEXT) | DPL_KERNEL) // 1000  => 0x8
#define KERNEL_DS ((G_D_K_DATA) | DPL_KERNEL) // 1010  => 0x10
#define USER_CS ((G_D_U_TEXT) | DPL_USER)	 // 0001 1011 => 0x1b
#define USER_DS ((G_D_U_DATA) | DPL_USER)	 // 0010 0011 => 0x23

/* *
 * Virtual memory map:                                          Permissions
 *                                                              kernel/user
 *
 *     4G ------------------> +---------------------------------+ 0xFFFFffff
 *                            |                                 |
 *                            |         Empty Memory (*)        |
 *                            |                                 |
 *                            +---------------------------------+ 0xFB000000
 *                            |   Cur. Page Table (Kern, RW)    | RW/-- PT_SIZE = 4MB
 *     VPT -----------------> +---------------------------------+ 0xFAC00000 
 *                            |        Invalid Memory (*)       | --/-- size = 44 MB
 *     KERN_TOP ------------> +---------------------------------+ 0xF8000000
 *                            |                                 |
 *                            |    Remapped Physical Memory     | RW/-- KMEMSIZE = 896MB
 *                            |                                 |
 *     KERN_BASE -----------> +---------------------------------+ 0xC0000000 (3GB)
*                             |        Invalid Memory (*)       | --/--
 *     USERTOP -------------> +---------------------------------+ 0xB0000000
 *                            |           User stack            |
 *                            +---------------------------------+
 *                            |                                 |
 *                            :                                 :
 *                            |         ~~~~~~~~~~~~~~~~        |
 *                            :                                 :
 *                            |                                 |
 *                            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                            |       User Program & Heap       |
 *     UTEXT ---------------> +---------------------------------+ 0x00800000
 *                            |        Invalid Memory (*)       | --/--
 *                            |  - - - - - - - - - - - - - - -  |
 *                            |    User STAB Data (optional)    |
 *     USERBASE, USTAB------> +---------------------------------+ 0x00200000
 *                            |        Invalid Memory (*)       | --/--
 *     0 -------------------> +---------------------------------+ 0x00000000
 *                            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * (*) Note: The kernel ensures that "Invalid Memory"(0xFAC00000 - 0xF8000000 = 
 *            0x2C00000) is never mapped. (0x2C00000 = 44MB)
 *     "Empty Memory" is normally unmapped, but user programs may map pages
 *     there if desired.
 *	 0xFFFFffff - 0xC0000000 = 0x3FFFFFFF ;
 * */

/* All physical memory mapped at this address */

// 最大 kernel virtual addr 的 KERN_TOP 的 PDE 虚拟地址为
// vpd + 0xf8000000 / 0x400000(4m) *4 = 0xFAFEB000 + 0x3E0 * 4 = 0xFAFEBF80

// 最大 kernel virtual addr 的 KERN_TOP 的 PTE 虚拟地址为
// vpt + 0xf8000000 / 0x1000(4k) *4 = 0xFAC00000 + 0xf8000 * 4 = 0xFAFE0000

// warning--> PDE & PTE 是4byte对齐.从下面的设置可看出KERN_TOP/4M 后的值是 4byte对齐的,
// 所以这样算出来的 PDE & PE 地址的最后两bit 一定是 0.

#define KERN_BASE 0xC0000000			//
#define KMEMSIZE 0x38000000				// 0x380 = 896 MB the maximum amount of physical memory  , os kernel 只支持 896MB 的物理内存空间, 这个 896MB 只是一个设定,可以根据情况改变.
#define KERN_TOP (KERN_BASE + KMEMSIZE) //最大的 kernel virtual addr = 0xf8000000

/* *
 * Virtual page table. Entry PDX[VPT] in the PD (Page Directory) contains
 * a pointer to the page directory itself, thereby turning the PD into a page
 * table, which maps all the PTEs (Page Table Entry) containing the page mappings
 * for the entire virtual address space into that 4 Meg region starting at VPT.
 * */
// high 10 bit = 1111 1010 11 = 1003(十进制) , 中间 10 bit = 0 , low 10 bit = 0
#define VPT 0xFAC00000

#define KERN_STACK_PAGE 2							  // # of pages in kernel stack
#define KERN_STACK_SIZE (KERN_STACK_PAGE * PAGE_SIZE) // sizeof kernel stack   8k


#define USERTOP             0xB0000000
#define USTACKTOP           USERTOP
#define USTACKPAGE          256				   //of pages in user stack
#define USTACKSIZE    		(USTACKPAGE*PAGE_SIZE)//sizeof user stack

#define USERBASE            0x00200000
#define UTEXT               0x00800000 	       //where user programs generally begin
#define USTAB               USERBASE		   //the location of the user STABS data structure

#define USER_ACCESS(start, end)                     \
	(USERBASE <= (start) && (start) < (end) && (end) <= USERTOP)

#define KERN_ACCESS(start, end)                     \
	(KERN_BASE <= (start) && (start) < (end) && (end) <= KERN_TOP)

#ifndef __ASSEMBLER__

#include <defs.h>
#include <atomic.h>
#include <list.h>

typedef uintptr_t pte_t;	// page table entry
typedef uintptr_t pde_t;	// page directory entry
typedef pte_t swap_entry_t; // the pte can also be a swap entry

/*
e820 is shorthand to refer to the facility by which the BIOS of x86-based computer systems reports the memory map to the operating system or boot loader.
It is accessed via the int 15h call, by setting the AX register to value E820 in hexadecimal. It reports which memory address ranges are usable and which are reserved for use by the BIOS.

(x86) e820 是一个可以侦测物理内存分布的硬件;
软件通过 15 号中断与之通信
*/
// some constants for bios interrupt 15h AX = 0xE820
/* Values for system memory map address type */
#define E820_MAX 20 // number of entries in E820MAP
#define E820_KERN 1 // address range memory, available to os
#define E820_SYS 2  // address range reserved,not available (sytem ROM, memory-mapped device)

struct e820map
{
	int nr_map;
	struct
	{
		uint64_t addr; // start of memory segment
		uint64_t size; // size of memory segment
		uint32_t type; // type of memory segment
	} __attribute__((packed)) map[E820_MAX];
};

/* 
 * struct Page - Page descriptor structures. Each Page describes one physical page.
 * 	             In kern/mm/pmm.h, you can find lots of useful functions that convert
 * 				 Page to other data types, such as physical address.
 * sizeof(struct Page)=20B
*/
struct Page
{
	int ref;			   // 映射此物理页的虚拟页个数 ;page frame's reference counter
	uint32_t flags;		   // 表示此物理页的状态标记,有两个标志位;array of flags that describe the status of the page frame
	unsigned int property; // the num of free block, used in first fit pm manager用来记录某连续内存空闲块的大小(即地址连续的空闲页的个数);这里需要注意的是用到此成员变量的这个 Page 比较特殊,是这个连续内存空闲块地址最小的一页(即头一页,Head Page).连续内存空闲块利用这个页的成员变量 property 来记录此块内的空闲页的个数. 不同分配算法,该字段含义不同.

	list_entry_t page_link;		// 把多个连续内存空闲块链接在一起的双向链表指针;free list link
	list_entry_t pra_page_link; // used for pra (page replace algorithm)
	uintptr_t pra_vaddr;		// used for pra (page replace algorithm)
};

/* Flags describing the status of a page frame */
#define PG_reserved 0 // the page descriptor is reserved for kernel or unusable 此页是否被保留, 该 bit =1,则为保留页,不能放到空闲链表中,即这样的页不是空闲页,不能动态分配与释放. 比如目前kernel code 占用的空间就是属于这样"被保留"的页.
#define PG_property 1 // the member 'property' is valid 该bit 表示此页是否是 free 的,如果设置为1,表示该页是 free 的,可以被分配; 如果设置0,表示该页已经被分配出去了,不能二次分配.  物理内存分配算法(best fit | buddy system |等)不同, PG_property 含义不同.

//将Page->flags 设置为 PG_reserved,表示这些页已经被使用了,将来不能被用于分配.
#define SetPageReserved(page) set_bit(PG_reserved, &((page)->flags))

#define ClearPageReserved(page) clear_bit(PG_reserved, &((page)->flags))
#define PageReserved(page) test_bit(PG_reserved, &((page)->flags))
#define SetPageProperty(page) set_bit(PG_property, &((page)->flags))
#define ClearPageProperty(page) clear_bit(PG_property, &((page)->flags))
#define PageProperty(page) test_bit(PG_property, &((page)->flags))

// convert list entry to page
#define le2page(le, member) \
	to_struct((le), struct Page, member)

/* free_area_t - maintains a doubly linked list to record free (unused) pages */
/* 一个双向链表,负责管理所有的连续内存空闲块,便于分配和释放 
  在初始情况下,也许这个physical mem 的空闲物理页都是连续的,这样就形成一个大的连续内存空闲块.
   但伴随着物理页的分配与释放,这个大的连续内存空闲块会分裂为一系列地址不连续的 多个小的连续内存空闲块,并且每个连续内存空闲块内部的物理页是连续的.    那么为了有效地管理这些小连续内存空闲块,所有的连续内存连续空闲块可用一个 双向链表管理起来, 便于分配和释放,为此定义了一个 struct free_area_t ,包含了一个 list_entry_t 的双向链表指针(指向了空闲的物理页).
*/

typedef struct
{
	list_entry_t free_list; // the list header
	unsigned int nr_free;   //记录当前空闲页的个数; # of free pages in this free list
} free_area_t;

// for slab style kmalloc
#define PG_slab                     2       // page frame is included in a slab
#define SetPageSlab(page)           set_bit(PG_slab, &((page)->flags))
#define ClearPageSlab(page)         clear_bit(PG_slab, &((page)->flags))
#define PageSlab(page)              test_bit(PG_slab, &((page)->flags))

#endif /* !__ASSEMBLER__ */

#endif

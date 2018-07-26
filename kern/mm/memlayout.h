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
#define SEG_K_TEXT    1
#define SEG_K_DATA    2
#define SEG_U_TEXT    3
#define SEG_U_DATA    4
#define SEG_K_TSS     5

/* global descriptor numbers */
#define G_D_K_TEXT    ((SEG_K_TEXT) << 3)        // kernel text   8(0x8)	=> (1000)
#define G_D_K_DATA    ((SEG_K_DATA) << 3)        // kernel data   16(0x10) => (1010)
#define G_D_U_TEXT    ((SEG_U_TEXT) << 3)        // user text     24(0x18) => (0001 1000)
#define G_D_U_DATA    ((SEG_U_DATA) << 3)        // user data  32(0x20) => (0010 0000)
#define G_D_TSS        ((SEG_K_TSS) << 3)// task segment selector 40(0x28) => (0010 1000)

#define DPL_KERNEL  (0)			// 0x0 => (0000)	
#define DPL_USER    (3)			// 0x3 => (0011)
/* segment sector (not segment descriptor)*/
#define KERNEL_CS    ((G_D_K_TEXT) | DPL_KERNEL) // 1000  => 0x8 
#define KERNEL_DS    ((G_D_K_DATA) | DPL_KERNEL) // 1010  => 0x10
#define USER_CS        ((G_D_U_TEXT) | DPL_USER) // 0001 1011 => 0x1b
#define USER_DS        ((G_D_U_DATA) | DPL_USER) // 0010 0011 => 0x23   


/* *
 * Virtual memory map:                                          Permissions
 *                                                              kernel/user
 *
 *     4G ------------------> +---------------------------------+
 *                            |                                 |
 *                            |         Empty Memory (*)        |
 *                            |                                 |
 *                            +---------------------------------+ 0xFB000000
 *                            |   Cur. Page Table (Kern, RW)    | RW/-- PTSIZE
 *     VPT -----------------> +---------------------------------+ 0xFAC00000
 *                            |        Invalid Memory (*)       | --/--
 *     KERNTOP -------------> +---------------------------------+ 0xF8000000
 *                            |                                 |
 *                            |    Remapped Physical Memory     | RW/-- KMEMSIZE
 *                            |                                 |
 *     KERNBASE ------------> +---------------------------------+ 0xC0000000
 *                            |                                 |
 *                            |                                 |
 *                            |                                 |
 *                            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * (*) Note: The kernel ensures that "Invalid Memory"(0xFAC00000 - 0xF8000000 = *
 *            0x2C00000) is *never* mapped.
 *     "Empty Memory" is normally unmapped, but user programs may map pages
 *     there if desired.
 *
 * */

/* All physical memory mapped at this address */
#define KERNBASE            0xC0000000  //
#define KMEMSIZE            0x38000000  // 0x380 = 896 MB the maximum amount of physical memory
#define KERNTOP             (KERNBASE + KMEMSIZE)

/* *
 * Virtual page table. Entry PDX[VPT] in the PD (Page Directory) contains
 * a pointer to the page directory itself, thereby turning the PD into a page
 * table, which maps all the PTEs (Page Table Entry) containing the page mappings
 * for the entire virtual address space into that 4 Meg region starting at VPT.
 * */
#define VPT                 0xFAC00000

#define KSTACKPAGE          2                           // # of pages in kernel stack
#define KSTACKSIZE          (KSTACKPAGE * PGSIZE)       // sizeof kernel stack


#ifndef __ASSEMBLER__

#include <defs.h>
#include <list.h>

typedef uintptr_t pte_t; // page table entry
typedef uintptr_t pde_t; // page  directory entry
/*
e820 is shorthand to refer to the facility by which the BIOS of x86-based computer systems reports the memory map to the operating system or boot loader.
It is accessed via the int 15h call, by setting the AX register to value E820 in hexadecimal. It reports which memory address ranges are usable and which are reserved for use by the BIOS.

(x86) e820 是一个可以侦测物理内存分布的硬件;
软件通过 15 号中断与之通信
*/
// some constants for bios interrupt 15h AX = 0xE820
#define E820MAX             20      // number of entries in E820MAP
#define E820_ARM            1       // address range memory
#define E820_ARR            2       // address range reserved

struct e820map
{
	int nr_map;
	struct {
		uint64_t addr; // start of memory segment
		uint64_t size; // size of memory segment
		uint32_t type; // type of memory segment
	} __attribute__((packed)) map[E820MAX];
};

/* 
 * struct Page - Page descriptor structures. Each Page describes one physical page.
 * 	             In kern/mm/pmm.h, you can find lots of useful functions that convert
 * 				 Page to other data types, such as physical address.
*/
struct Page 
{
    int ref; // 映射此物理页的虚拟页个数 ;page frame's reference counter
    uint32_t flags; // 表示此物理页的状态标记,有两个标志位;array of flags that describe the status of the page frame
    unsigned int property; // 用来记录某连续内存空闲块的大小;the num of free block, used in first fit pm manager
    list_entry_t page_link;// 把多个连续内存空闲块链接在一起的双向链表指针;free list link
};

/* Flags describing the status of a page frame */
#define PG_reserved                 0       // the page descriptor is reserved for kernel or unusable
#define PG_property                 1       // the member 'property' is valid

#define SetPageReserved(page)       set_bit(PG_reserved, &((page)->flags))
#define ClearPageReserved(page)     clear_bit(PG_reserved, &((page)->flags))
#define PageReserved(page)          test_bit(PG_reserved, &((page)->flags))
#define SetPageProperty(page)       set_bit(PG_property, &((page)->flags))
#define ClearPageProperty(page)     clear_bit(PG_property, &((page)->flags))
#define PageProperty(page)          test_bit(PG_property, &((page)->flags))

// convert list entry to page
#define le2page(le, member)                 \
    to_struct((le), struct Page, member)

/* free_area_t - maintains a doubly linked list to record free (unused) pages */
/* 一个双向链表,负责管理所有的连续内存空闲块,便于分配和释放 */
typedef struct {
    list_entry_t free_list;// the list header
    unsigned int nr_free;  //记录当前空闲页的个数; # of free pages in this free list
} free_area_t;

#endif /* !__ASSEMBLER__ */

#endif

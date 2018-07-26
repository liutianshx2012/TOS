/*************************************************************************
	> File Name: pmm.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 11时50分25秒
 ************************************************************************/
#ifndef _KERN_MM_PMM_H
#define _KERN_MM_PMM_H

#include <defs.h>
#include <mmu.h>
#include <memlayout.h>

/* pmm_manager  is a physical memory management struct. A special pmm manager - 
 * xxx_pmm_manager . only needs to implement the methods in pmm_manager class, then
 * xxx_pmm_manager can be used by kernel to manager the total physical memory space.
*/
struct pmm_manager 
{
	const char *name;	// XXX_pmm_manager's name
	void (*init)(void); // initialize internal description & management data structure
						// (free block list, number of free block) of XXX_pmm_manager
	void (*init_memmap)(struct Page *base, size_t n);// setup description&management data structcure according to the initial free physical memory space
	
	struct Page *(*alloc_pages)(size_t n);            // allocate >=n pages, depend on the allocation algorithm
    void (*free_pages)(struct Page *base, size_t n);  // free >=n pages with "base" addr of Page descriptor structures(memlayout.h)
    size_t (*nr_free_pages)(void);                    // return the number of free pages
    void (*check)(void);                              // check the correctness of XXX_pmm_manager
}

extern const struct pmm_manager *pmm_manager;

extern pde_t *boot_pgdir;

extern uintptr_t boot_cr3;

void pmm_init(void);

#endif

/*************************************************************************
	> File Name: swapfs.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Fri 08 Jul 2016 02:51:57 PM CST
 ************************************************************************/
#ifndef __KERN_FS_SWAPFS_H__
#define __KERN_FS_SWAPFS_H__

#include <memlayout.h>
#include <swap.h>

void swapfs_init(void);

int swapfs_read(swap_entry_t entry, struct Page *page);

int swapfs_write(swap_entry_t entry, struct Page *page);

#endif /* !__KERN_FS_SWAPFS_H__ */


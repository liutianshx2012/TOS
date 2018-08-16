/*************************************************************************
	> File Name: kmalloc.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Mon 18 Jul 2016 10:50:01 PM CST
 ************************************************************************/
#ifndef __KERN_MM_KMALLOC_H
#define __KERN_MM_KMALLOC_H

#include <defs.h>
/*
* 定义和实现了新的 kmalloc|kfree 函数.基于 slab 内存分配器.
*/

#define KMALLOC_MAX_ORDER       10

void kmalloc_init(void);

void *kmalloc(size_t n);

void kfree(void *objp);

size_t kallocated(void);


#endif

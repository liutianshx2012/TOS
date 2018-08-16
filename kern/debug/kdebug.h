/*************************************************************************
	> File Name: kdebug.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 14时33分25秒
 ************************************************************************/
#ifndef __KERN_DEBUG_KDEBUG_H
#define __KERN_DEBUG_KDEBUG_H

#include <defs.h>

void print_kerninfo(void);

void print_stackframe(void);

void print_debuginfo(uintptr_t eip);

#endif

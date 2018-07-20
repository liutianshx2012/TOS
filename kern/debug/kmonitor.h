/*************************************************************************
	> File Name: kmonitor.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 14时36分53秒
 ************************************************************************/
#ifndef _KERN_DEBUG_KMONITOR_H
#define _KERN_DEBUG_KMONITOR_H

#include <trap.h>

void kmonitor(struct trapframe *tf);

int mon_help(int argc, char **argv, struct trapframe *tf);

int mon_kerninfo(int argc, char **argv, struct trapframe *tf);

int mon_backtrace(int argc, char **argv, struct trapframe *tf);

#endif

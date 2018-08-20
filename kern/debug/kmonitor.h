/*************************************************************************
	> File Name: kmonitor.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 14时36分53秒
 ************************************************************************/
#ifndef __KERN_DEBUG_KMONITOR_H
#define __KERN_DEBUG_KMONITOR_H

#include <trap.h>

void kmonitor(struct trapframe *tf);

int mon_help(int argc, char **argv, struct trapframe *tf);

int mon_kerninfo(int argc, char **argv, struct trapframe *tf);

int mon_backtrace(int argc, char **argv, struct trapframe *tf);

int mon_continue(int argc, char **argv, struct trapframe *tf);

int mon_step(int argc, char **argv, struct trapframe *tf);

int mon_breakpoint(int argc, char **argv, struct trapframe *tf);

int mon_watchpoint(int argc, char **argv, struct trapframe *tf);

int mon_delete_dr(int argc, char **argv, struct trapframe *tf);

int mon_list_dr(int argc, char **argv, struct trapframe *tf);

#endif

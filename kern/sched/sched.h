/*************************************************************************
	> File Name: sched.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Mon 18 Jul 2016 10:48:42 PM CST
 ************************************************************************/
#ifndef __KERN_SCHED_SCHED_H
#define __KERN_SCHED_SCHED_H

#include <proc.h>

void schedule(void);

void wakeup_proc(struct proc_struct *proc);

#endif


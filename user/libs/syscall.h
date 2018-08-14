/*************************************************************************
	> File Name: syscall.h
	> Author: TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Mon 01 Aug 2016 08:00:54 PM PDT
 ************************************************************************/
#ifndef _USER_LIBS_SYSCALL_H
#define _USER_LIBS_SYSCALL_H

int sys_exit(int error_code);

int sys_fork(void);

int sys_wait(int pid,int *store);

int sys_yield(void);

int sys_kill(int pid);

int sys_getpid(void);

int sys_putc(int c);

int sys_pgdir(void);

#endif


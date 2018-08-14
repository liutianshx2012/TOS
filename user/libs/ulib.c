/*************************************************************************
	> File Name: ulib.c
	> Author: TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Mon 01 Aug 2016 07:59:02 PM PDT
 ************************************************************************/
#include <defs.h>
#include <stdio.h>
#include <syscall.h>
#include <ulib.h>

void
exit(int error_code) 
{
    sys_exit(error_code);
    cprintf("Bug: exit faild.\n");
    while(1);
}

int 
fork(void) 
{
    return sys_fork();
}

int
wait(void) 
{
    return sys_wait(0,NULL);
}

int 
waitpid(int pid, int *store) 
{
    return sys_wait(pid,store);
}

void
yield(void) 
{
    sys_yield();
}

int
kill(int pid) 
{
    return sys_kill(pid);
}

int
getpid(void) 
{
    return sys_getpid();
}

//print_pgdir -- print the PDT & PT
void 
print_pgdir(void) 
{
    sys_pgdir();
}

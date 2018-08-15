/*************************************************************************
	> File Name: syscall.c
	> Author: TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Mon 01 Aug 2016 08:04:20 PM PDT
 ************************************************************************/
#include <defs.h>
#include <unistd.h>
#include <stdarg.h>
#include <syscall.h>

#define MAX_ARGS   5

static inline int
syscall(int num, ...) 
{
    va_list ap;
    va_start(ap,num);
    uint32_t a[MAX_ARGS];
    int i,ret;
    for (i =0; i <MAX_ARGS; i++) {
        a[i] = va_arg(ap,uint32_t);
    }
    va_end(ap);
    // eax 存中断号, a[0]~a[4] 分别保存在 EDX | ECX | EBX | EDI | ESI 五个寄存器中.
    // 最多用 6 个寄存器来传递系统调用的参数, 系统调用返回结果存在 EAX 中.
    __asm__ volatile (
        "int %1;"
        : "=a" (ret)
        : "i" (T_SYSCALL),
            "a" (num),
            "d" (a[0]),
            "c" (a[1]),
            "b" (a[2]),
            "D" (a[3]),
            "S" (a[4])
        : "cc", "memory"
    );

    return ret;
}


int
sys_exit(int error_code) 
{
    return syscall(SYS_exit,error_code);
}

int
sys_fork(void) 
{
    return syscall(SYS_fork);
}

int 
sys_wait(int pid, int *store) 
{
    return syscall(SYS_wait,pid,store);
}

int
sys_yield(void) 
{
    return syscall(SYS_yield);
}

int
sys_kill(int pid) 
{
    return syscall(SYS_kill,pid);
}

int 
sys_getpid(void) 
{
    return syscall(SYS_getpid);
}

int
sys_putc(int c) 
{
    return syscall(SYS_putc, c);
}

int
sys_pgdir(void) {
    return syscall(SYS_pgdir);
}




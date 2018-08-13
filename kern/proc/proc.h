/*************************************************************************
	> File Name: proc.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Mon 18 Jul 2016 10:44:31 PM CST
 ************************************************************************/
#ifndef _KERN_PROC_PROC_H
#define _KERN_PROC_PROC_H

#include <defs.h>
#include <list.h>
#include <trap.h>
#include <memlayout.h>

// 实现进程|线程相关功能 ==>  创建进程|线程, 初始化进程|线程, 处理进程|线程退出

// process's state in his life cycle
enum proc_state
{
    PROC_UNINIT = 0, // uninitialized
    PROC_SLEEPING,   // sleeping
    PROC_RUNNABLE,   // runnable(maybe running)
    PROC_ZOMBIE,     // almost dead, and wait parent proc to reclaim his resource
};

// Saved registers for kernel context switches.
// Don't need to save all the %fs etc. segment registers,
// because they are constant across kernel contexts.
// Save all the regular registers so we don't need to care
// which are caller save, but not the return register %eax.
// (Not saving %eax just simplifies the switching code.)
// The layout of context must match code in switch.S.
struct context
{
    uint32_t eip;
    uint32_t esp;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
};

#define PROC_NAME_LEN 15
#define MAX_PROCESS 4096
#define MAX_PID (MAX_PROCESS * 2)

extern list_entry_t proc_list;

struct proc_struct
{
    enum proc_state state;        // Process state
    int pid;                      // Process ID
    int runs;                     // the running times of Proces
    uintptr_t kstack;             // Process kernel stack
    volatile bool need_resched;   // bool value: need to be rescheduled to release CPU?
    struct proc_struct *parent;   // the parent process
    struct mm_struct *mm;         // Process's memory management field
    struct context context;       // Switch here to run process
    struct trapframe *tf;         // Trap frame for current interrupt
    uintptr_t cr3;                // CR3 register: the base addr of PDT
    uint32_t flags;               // Process flag
    char name[PROC_NAME_LEN + 1]; // Process name
    list_entry_t list_link;       // Process link list
    list_entry_t hash_link;       // Process hash list
};

#define le2proc(le, member) \
    to_struct((le), struct proc_struct, member)

extern struct proc_struct *idleproc, *initproc, *current;

void proc_init(void);

void proc_run(struct proc_struct *proc);

int kernel_thread(int (*fn)(void *), void *arg, uint32_t clone_flags);

char *set_proc_name(struct proc_struct *proc, const char *name);

char *get_proc_name(struct proc_struct *proc);

void cpu_idle(void) __attribute__((noreturn));

struct proc_struct *find_proc(int pid);

int do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf);

int do_exit(int error_code);

#endif
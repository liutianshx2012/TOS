/*************************************************************************
	> File Name: proc.c
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Mon 18 Jul 2016 10:44:39 PM CST
 ************************************************************************/
#include <kmalloc.h>
#include <string.h>
#include <sync.h>
#include <pmm.h>
#include <error.h>
#include <sched.h>
#include <elf.h>
#include <vmm.h>
#include <trap.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <proc.h>

/* ------------- process/thread mechanism design & implementation -------------
(an simplified Linux process/thread mechanism )
introduction:
  OS implements a simple process/thread mechanism. process contains the independent memory sapce, at least one threads
for execution, the kernel data(for management), processor state (for context switch), files(in proj6), etc. OS needs to
manage all these details efficiently. In OS, a thread is just a special kind of process(share process's memory).
------------------------------
process state       :     meaning               -- reason
    PROC_UNINIT     :   uninitialized           -- alloc_proc
    PROC_SLEEPING   :   sleeping                -- try_free_pages, do_wait, do_sleep
    PROC_RUNNABLE   :   runnable(maybe running) -- proc_init, wakeup_proc, 
    PROC_ZOMBIE     :   almost dead             -- do_exit

-----------------------------
process state changing:
                                            
  alloc_proc                                 RUNNING
      +                                   +--<----<--+
      +                                   + proc_run +
      V                                   +-->---->--+ 
PROC_UNINIT -- proc_init/wakeup_proc --> PROC_RUNNABLE -- try_free_pages/do_wait/do_sleep --> PROC_SLEEPING --
                                           A      +                                                           +
                                           |      +--- do_exit --> PROC_ZOMBIE                                +
                                           +                                                                  + 
                                           -----------------------wakeup_proc----------------------------------
-----------------------------
process relations
parent:           proc->parent  (proc is children)
children:         proc->cptr    (proc is parent)
older sibling:    proc->optr    (proc is younger sibling)
younger sibling:  proc->yptr    (proc is older sibling)
-----------------------------
related syscall for process:
SYS_exit        : process exit,                           -->do_exit
SYS_fork        : create child process, dup mm            -->do_fork-->wakeup_proc
SYS_wait        : wait process                            -->do_wait
SYS_exec        : after fork, process execute a program   -->load a program and refresh the mm
SYS_clone       : create child thread                     -->do_fork-->wakeup_proc
SYS_yield       : process flag itself need resecheduling, -- proc->need_sched=1, then scheduler will rescheule this process
SYS_sleep       : process sleep                           -->do_sleep 
SYS_kill        : kill process                            -->do_kill-->proc->flags |= PF_EXITING
                                                                 -->wakeup_proc-->do_wait-->do_exit   
SYS_getpid      : get the process's pid

*/

// the process set's list
list_entry_t proc_list;

#define HASH_SHIFT          10
#define HASH_LIST_SIZE      (1 << HASH_SHIFT)
#define pid_hashfn(x)       (hash32(x, HASH_SHIFT))

// hash list for process set based on pid
static list_entry_t hash_list[HASH_LIST_SIZE];

// idle proc 第 0 号 kernel thread
struct proc_struct *idleproc = NULL;
// init proc 第 1 号 kernel thread
struct proc_struct *initproc = NULL;
// current proc
struct proc_struct *current = NULL;
// numbers of running process
static int nr_process = 0;

void kernel_thread_entry(void);
void forkrets(struct trapframe *tf);
void switch_to(struct context *from, struct context *to);

// alloc_proc - alloc a proc_struct and init all fields of proc_struct
static struct proc_struct *
alloc_proc(void) 
{
    struct proc_struct *proc = kmalloc(sizeof(struct proc_struct));

    if (proc != NULL) {
        proc->state = PROC_UNINIT;
        proc->pid = -1;
        proc->runs = 0;
        proc->kstack = 0;
        proc->need_resched = 0;
        proc->parent = NULL;
        proc->mm = NULL;
        memset(&(proc->context), 0, sizeof(struct context));
        proc->tf = NULL;
        proc->cr3 = boot_cr3;
        proc->flags = 0;
        memset(proc->name, 0, PROC_NAME_LEN);
        proc->wait_state = 0;
        proc->cptr = proc->optr = proc->yptr = NULL;

        proc->rq = NULL;
        list_init(&(proc->run_link));
        proc->time_slice = 0;
        proc->proj6_run_pool.left = proc->proj6_run_pool.right =   proc->proj6_run_pool.parent = NULL;
        proc->proj6_stride = 0;
        proc->proj6_priority = 0;
    }

    return proc;
}

// set_proc_name - set the name of proc
char *
set_proc_name(struct proc_struct *proc, const char *name) 
{
    memset(proc->name, 0, sizeof(proc->name));
    return memcpy(proc->name, name, PROC_NAME_LEN);
}

// get_proc_name - get the name of proc
char *
get_proc_name(struct proc_struct *proc) 
{
    static char name[PROC_NAME_LEN + 1];
    memset(name, 0, sizeof(name));
    return memcpy(name, proc->name, PROC_NAME_LEN);
}

//set_links - set the relation links of process
static void
set_links(struct proc_struct *proc) 
{
    list_add(&proc_list, &(proc->list_link));
    proc->yptr = NULL;
    if ((proc->optr = proc->parent->cptr) != NULL) {
        proc->optr->yptr = proc;
    }
    proc->parent->cptr = proc;
    nr_process ++;
}
//remove_links -- clean the relation links of process
static void
remove_links(struct proc_struct *proc) 
{
    list_del(&(proc->list_link));
    if (proc->optr != NULL) {
        proc->optr->yptr = proc->yptr;
    }
    if (proc->yptr != NULL) {
        proc->yptr->optr = proc->optr;
    } else {
       proc->parent->cptr = proc->optr;
    }
    nr_process --;
}

// get_pid - alloc a unique pid for process
static int
get_pid(void) 
{
    static_assert(MAX_PID > MAX_PROCESS);
    struct proc_struct *proc;
    list_entry_t *list = &proc_list, *le;
    static int next_safe = MAX_PID, last_pid = MAX_PID;
    if (++ last_pid >= MAX_PID) {
        last_pid = 1;
        goto inside;
    }
    if (last_pid >= next_safe) {
    inside:
        next_safe = MAX_PID;
    repeat:
        le = list;
        while ((le = list_next(le)) != list) {
            proc = le2proc(le, list_link);
            if (proc->pid == last_pid) {
                if (++ last_pid >= next_safe) {
                    if (last_pid >= MAX_PID) {
                        last_pid = 1;
                    }
                    next_safe = MAX_PID;
                    goto repeat;
                }
            } else if (proc->pid > last_pid && next_safe > proc->pid) {
                next_safe = proc->pid;
            }
        }
    }

    return last_pid;
}

// proc_run - make process "proc" running on cpu
// NOTE: before call switch_to, should load base addr of "proc"'s new PDT
void
proc_run(struct proc_struct *proc) 
{
    if (proc != current) {
        
        

        bool intr_flag;
		struct proc_struct *prev = current, *next = proc;
        local_intr_save(intr_flag);
        {
            //设置当前进程
            current = proc;
            //设置任务状态段 ts 中特权态 0 下的 esp0 为 next 内核线程 initproc 的内核栈的栈顶
            load_esp0(next->kstack + KERN_STACK_SIZE);
            //完成进程间的页表切换,设置 CR3 为 next 内核线程 initproc 的 PDT 起始地址 
            lcr3(next->cr3);
            //完成具体的两个线程的执行现场切换,即切换各个寄存器
            switch_to(&(prev->context), &(next->context));
        }
        local_intr_restore(intr_flag);
    }
}

// forkret -- the first kernel entry point of a new thread/process
// NOTE: the addr of forkret is setted in copy_thread function
//       after switch_to, the current proc will execute here.
static void
forkret(void) 
{
    forkrets(current->tf);
}

// hash_proc - add proc into proc hash_list
static void
hash_proc(struct proc_struct *proc) 
{
    list_add(hash_list + pid_hashfn(proc->pid), &(proc->hash_link));
}
// unhash_proc -delete proc from proc hash_list
static void
unhash_proc(struct proc_struct *proc) 
{
    list_del(&(proc->hash_link));  
}

// find_proc - find proc from proc hash_list according to pid
struct proc_struct *
find_proc(int pid)
{
    if (0 < pid && pid < MAX_PID) {
        list_entry_t *list = hash_list + pid_hashfn(pid);
        list_entry_t *le = list;
        while ((le = list_next(le)) != list) {
            struct proc_struct *proc = le2proc(le, hash_link);
            if (proc->pid == pid) {
                return proc;
            }
        }
    }

    return NULL;
}

// kernel_thread - create a kernel thread using "fn" function
// NOTE: the contents of temp trapframe tf will be copied to 
//       proc->tf in do_fork-->copy_thread function
int
kernel_thread(int (*fn)(void *), void *arg, uint32_t clone_flags) 
{
    struct trapframe tf;
    memset(&tf, 0, sizeof(struct trapframe));
    tf.tf_cs = KERNEL_CS;
    tf.tf_ds = tf.tf_es = tf.tf_ss = KERNEL_DS;
    tf.tf_regs.reg_ebx = (uint32_t)fn;
    tf.tf_regs.reg_edx = (uint32_t)arg;
    tf.tf_eip = (uint32_t)kernel_thread_entry;

    return do_fork(clone_flags | CLONE_VM, 0, &tf);
}

// setup_kstack - alloc pages with size KERN_STACK_PAGE as process kernel stack
static int
setup_kstack(struct proc_struct *proc) 
{
    struct Page *page = alloc_pages(KERN_STACK_PAGE);
    if (page != NULL) {
        proc->kstack = (uintptr_t)page2kva(page);
        return 0;
    }
    return -E_NO_MEM;
}

// put_kstack - free the memory space of process kernel stack
static void
put_kstack(struct proc_struct *proc) 
{
    free_pages(kva2page((void *)(proc->kstack)), KERN_STACK_PAGE);
}

// setup_pgdir - alloc one page as PDT
// 申请一个 PDT 所需的一个 page 内存,并把描述 kernel 虚拟空间映射的 内核 PT(boot_pgdir所指)
// 的内容 copy 到此新 PDT 中, 最后让 mm->pgdir 指向此 PDT (该进程的新的 PDT),且能够正确映射
// 内核虚拟空间.
static int
setup_pgdir(struct mm_struct *mm) 
{
    struct Page *page;
    if ((page = alloc_page()) == NULL) {
        return -E_NO_MEM;
    }
    pde_t *pgdir = page2kva(page);
    memcpy(pgdir, boot_pgdir, PAGE_SIZE);
    pgdir[PDE_X(VPT)] = PADDR(pgdir) | PTE_P | PTE_W;
    mm->pgdir = pgdir;

    return 0;
}

// put_pgdir - free the memory space of PDT
// 释放当前进程的 PDT 所占的内存
static void
put_pgdir(struct mm_struct *mm) 
{
    free_page(kva2page(mm->pgdir));
}

// copy_mm - process "proc" duplicate OR share process "current"'s mm according clone_flags
//         - if clone_flags & CLONE_VM, then "share" ; else "duplicate"
static int
copy_mm(uint32_t clone_flags, struct proc_struct *proc) 
{
    struct mm_struct *mm, *oldmm = current->mm;

    /* current is a kernel thread */
    if (oldmm == NULL) {
        return 0;
    }
    if (clone_flags & CLONE_VM) {
        mm = oldmm;
        goto good_mm;
    }

    int ret = -E_NO_MEM;
    if ((mm = mm_create()) == NULL) {
        goto bad_mm;
    }
    if (setup_pgdir(mm) != 0) {
        goto bad_pgdir_cleanup_mm;
    }

    lock_mm(oldmm);
    {
        ret = dup_mmap(mm, oldmm);
    }
    unlock_mm(oldmm);

    if (ret != 0) {
        goto bad_dup_cleanup_mmap;
    }

good_mm:
    mm_count_inc(mm);
    proc->mm = mm;
    proc->cr3 = PADDR(mm->pgdir);
    return 0;
bad_dup_cleanup_mmap:
    exit_mmap(mm);
    put_pgdir(mm);
bad_pgdir_cleanup_mm:
    mm_destroy(mm);
bad_mm:
    return ret;
}

// copy_thread - setup the trapframe on the  process's kernel stack top and
//             - setup the kernel entry point and stack of process
static void
copy_thread(struct proc_struct *proc, uintptr_t esp, struct trapframe *tf) 
{
    cprintf("when crossing rings(U->K)  proc->tf->tf_esp: [0x%08lx]\n",esp);
    // 在内核堆栈的顶部设置中断帧大小的一块栈空间
    proc->tf = (struct trapframe *)(proc->kstack + KERN_STACK_SIZE) - 1;
    // 拷贝在 kernel_thread 函数建立的临时中断帧的初始值
    *(proc->tf) = *tf; 
    proc->tf->tf_regs.reg_eax = 0;
    //设置子进程/线程执行完 do_fork 后的返回值
    proc->tf->tf_esp = esp; //设置中断帧中的栈指针esp
    proc->tf->tf_eflags |= FL_IF;//设置 EFLAGS ,使能中断
    /*
    fork 进程的中断帧就建立好了.
    initproc->tf= (proc->kstack+KERN_STACK_SIZE) – sizeof (struct trapframe);
    //具体内容
    initproc->tf.tf_cs = KERNEL_CS;
    initproc->tf.tf_ds = initproc->tf.tf_es = initproc->tf.tf_ss = KERNEL_DS;
    initproc->tf.tf_regs.reg_ebx = (uintptr_t)init_main;
    initproc->tf.tf_regs.reg_edx = (uintptr_t) ADDRESS of "Helloworld!!";
    initproc->tf.tf_eip = (uintptr_t)kernel_thread_entry;
    initproc->tf.tf_regs.reg_eax = 0;
    initproc->tf.tf_esp = esp;
    initproc->tf.tf_eflags |= FL_IF;
    */
    
    /*
    设置好中断帧后,最后就是设置 initproc 的进程上下文(proc context, 执行现场).
    有设置好执行现场后,一旦 kernel scheder选择了 initproc 执行,就需要根据 initproc->context 
    中保存的执行现场来恢复 initproc 的执行.

    执行现场中主要的两个信息: 上次停止执行时的下一条指令地址 context.eip 和上
    次停止执行时的堆栈地址 context.esp, 其实 initproc 还没有执行过,所以这其实就是 initproc 实际
    执行的第一条指令地址和堆栈指针.
    
    可以看出,由于 initproc 的中断帧占用了实际给 initproc 分配的栈空间的顶部,
    所以 initproc 就只能把栈顶指针 context.esp 设置在 initproc 的中断帧的起始位置.
    根据 context.eip 的赋值,可以知道 initproc 实际开始执行的地方在 forkret 函数处.至此,initproc 内核线程已经做好准备执行了.
    */
    proc->context.eip = (uintptr_t)forkret;
    proc->context.esp = (uintptr_t)(proc->tf);    
}

/* do_fork -     parent process for a new child process
 * @clone_flags: used to guide how to clone the child process
 * @stack:       the parent's user stack pointer. if stack==0, It means to fork a kernel thread.
 * @tf:          the trapframe info, which will be copied to child process's proc->tf
 */
int
do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf) 
{
    int ret = -E_NO_FREE_PROC;
    struct proc_struct *proc;
    if (nr_process >= MAX_PROCESS) {
        goto fork_out;
    }
    ret = -E_NO_MEM;
    //    1. call alloc_proc to allocate a proc_struct
    //    2. call setup_kstack to allocate a kernel stack for child process
    //    3. call copy_mm to dup OR share mm according clone_flag
    //    4. call copy_thread to setup tf & context in proc_struct
    //    5. insert proc_struct into hash_list && proc_list
    //    6. call wakeup_proc to make the new child process RUNNABLE
    //    7. set ret vaule using child proc's pid
    if ((proc = alloc_proc()) == NULL) {
        goto fork_out;
    }

    proc->parent = current;
    assert(current->wait_state == 0);

    if (setup_kstack(proc) != 0) {
        goto bad_fork_cleanup_proc;
    }
    if (copy_mm(clone_flags, proc) != 0) {
        goto bad_fork_cleanup_kstack;
    }
    copy_thread(proc, stack, tf);

    bool intr_flag;
    local_intr_save(intr_flag);
    {
        proc->pid = get_pid();
        hash_proc(proc);
        set_links(proc);
    }
    local_intr_restore(intr_flag);

    wakeup_proc(proc);

    ret = proc->pid;
fork_out:
    return ret;

bad_fork_cleanup_kstack:
    put_kstack(proc);
bad_fork_cleanup_proc:
    kfree(proc);
    goto fork_out;
}

// do_exit - called by sys_exit
//   1. call exit_mmap & put_pgdir & mm_destroy to free the almost all memory space of processπ
//   2. set process' state as PROC_ZOMBIE, then call wakeup_proc(parent) to ask parent reclaim itself.
//   3. call scheduler to switch to other process
// 释放进程自身所占内存空间和相关内存管理(PT)信息所占内存,
// 唤醒父进程, 好让父进程收了自己,让调度器切换到其它进程.
int
do_exit(int error_code) 
{
    cprintf("do_exit error_code [%d]\n",error_code);
    if (current == idleproc) {
        panic("idleproc exit.\n");
    }
    if (current == initproc) {
        panic("initproc exit.\n");
    }

    struct mm_struct *mm = current->mm;
    if (mm != NULL) {
        // 切换到内核态的 PT 上,这样当前用户进程目前只能在内核虚拟地址空间执行了,这是为了
        // 确保后续释放用户态内存 和 用户态 PT 的工作能够正常执行.
        lcr3(boot_cr3);
        // 当前进程 mm 的引用计数为 0 表示没有其它进程共享,可以彻底释放进程所占的用户虚拟空间了。
        if (mm_count_dec(mm) == 0) {
            exit_mmap(mm);
            put_pgdir(mm);
            mm_destroy(mm);
        }
        // 表示当前进程相关的用户虚拟内存空间 和 对应的内存管理成员变量所占内核虚拟内存空间 已经回收完毕.
        current->mm = NULL;
    }
    // 当前进程已经不能再被调度器 调度执行
    current->state = PROC_ZOMBIE;
    current->exit_code = error_code;

    bool intr_flag;
    struct proc_struct *proc;
    local_intr_save(intr_flag);
    {
        // 如果当前进程的父进程处于等待子进程状态则唤醒父进程,让父进程帮子进程完成最后的资源回收.
        proc = current->parent;
        if (proc->wait_state == WT_CHILD) {
            wakeup_proc(proc);
        }
        // 如果当前进程还有子进程,则需要把这些子进程的父进程指针设置为 内核线程 initproc(现代 OS 
        // 的 init 进程),且各个子进程指针需要插入到 initproc 的子进程链表中。 
        // 如果某个子进程的执行状态是 PROC_ZOMBIE,则需要唤醒 initproc 来完成对此子进程的最后回收
        while (current->cptr != NULL) {
            proc = current->cptr;
            current->cptr = proc->optr;

            proc->yptr = NULL;
            if ((proc->optr = initproc->cptr) != NULL) {
                initproc->cptr->yptr =proc;
            }
            proc->parent = initproc;
            initproc->cptr = proc;
            if (proc->state == PROC_ZOMBIE) {
                if (initproc->wait_state == WT_CHILD) {
                    wakeup_proc(initproc);
                }
            }
        }
    }
    local_intr_restore(intr_flag);
    // 重新选择新的 process 调度执行
    schedule();
    panic("do_exit will not return!! %d.\n", current->pid);
}

// do_yield - ask the scheduler to reschedule
// 让调度器执行一次选择新进程的过程.
int
do_yield(void) 
{
    current->need_resched = 1;
    return 0;
}

/* load_elf - load the content of binary program(ELF format) as the new content of current process
 * @binary:  the memory addr of the content of binary program
 * @size:  the size of the content of binary program
 * 完成加载放在内存中的执行程序到进程空间,涉及对页表的修改,分配用户栈.
 */
static int
load_elf(unsigned char *binary, size_t size) 
{
    // cprintf("load use ELF [%s] size [%d]\n",binary,size);
    if (current->mm != NULL) {
        panic("load_elf: current->mm must be empty.\n");
    }

    int ret = -E_NO_MEM;
    struct mm_struct *mm;
    //(1) create a new mm for current process
    // 申请进程的内存管理数据结构 mm 所需内存空间.
    if ((mm = mm_create()) == NULL) {
        goto bad_mm;
    }

    //(2) create a new PDT, and mm->pgdir= kernel virtual addr of PDT
    if (setup_pgdir(mm) != 0) {
        goto bad_pgdir_cleanup_mm;
    }

    // 根据执行程序各个段的大小分配物理内存空间,并根据执行程序各个段的起始位置确定虚拟地址,并在 PT 中
    // 建立好 pa 与 va 的映射关系.
    // 然后把执行程序各个段的内容 copy 到相应的内核虚拟地址中,至此 执行程序的代码与数据根据编译时设定
    // 地址(va) 放置到了虚拟内存中.
    //(3) copy TEXT/DATA section, build BSS parts in binary to memory space of process
    struct Page *page;
    //(3.1) get the file header of the bianry program (ELF format)
    struct elfhdr *elf = (struct elfhdr *)binary;
    //(3.2) get the entry of the program section headers of the bianry program (ELF format)
    struct proghdr *ph = (struct proghdr *)(binary + elf->e_phoff);
    //(3.3) This program is valid?
    if (elf->e_magic != ELF_MAGIC) {
        ret = -E_INVAL_ELF;
        goto bad_elf_cleanup_pgdir;
    }

    uint32_t vm_flags, perm;
    struct proghdr *ph_end = ph + elf->e_phnum;
    for (; ph < ph_end; ph ++) {
    //(3.4) find every program section headers
        if (ph->p_type != ELF_PT_LOAD) {
            continue ;
        }
        if (ph->p_filesz > ph->p_memsz) {
            ret = -E_INVAL_ELF;
            goto bad_cleanup_mmap;
        }
        if (ph->p_filesz == 0) {
            continue ;
        }
    //(3.5) call mm_map fun to setup the new vma ( ph->p_va, ph->p_memsz)
        vm_flags = 0, perm = PTE_U;
        if (ph->p_flags & ELF_PF_X) {
            vm_flags |= VM_EXEC;
        }
        if (ph->p_flags & ELF_PF_W) {
            vm_flags |= VM_WRITE;
        }
        if (ph->p_flags & ELF_PF_R) {
            vm_flags |= VM_READ;
        }
        if (vm_flags & VM_WRITE) {
            perm |= PTE_W;
        }
        ret = mm_map(mm, ph->p_va, ph->p_memsz, vm_flags, NULL);

        if (ret!= 0) {
            goto bad_cleanup_mmap;
        }
        unsigned char *from = binary + ph->p_offset;
        size_t off, size;
        uintptr_t start = ph->p_va, end, la = ROUNDDOWN(start, PAGE_SIZE);

        ret = -E_NO_MEM;

     //(3.6) alloc memory, and  copy the contents of every program section (from, from+end) to process's memory (la, la+end)
        end = ph->p_va + ph->p_filesz;
     //(3.6.1) copy TEXT/DATA section of bianry program
        while (start < end) {
            if ((page = pgdir_alloc_page(mm->pgdir, la, perm)) == NULL) {
                goto bad_cleanup_mmap;
            }
            off = start - la, size = PAGE_SIZE - off, la += PAGE_SIZE;
            if (end < la) {
                size -= la - end;
            }
            memcpy(page2kva(page) + off, from, size);
            start += size, from += size;
        }

      //(3.6.2) build BSS section of binary program
        end = ph->p_va + ph->p_memsz;
        if (start < la) {
            /* ph->p_memsz == ph->p_filesz */
            if (start == end) {
                continue ;
            }
            off = start + PAGE_SIZE - la, size = PAGE_SIZE - off;
            if (end < la) {
                size -= la - end;
            }
            memset(page2kva(page) + off, 0, size);
            start += size;
            assert((end < la && start == end) || (end >= la && start == la));
        }
        while (start < end) {
            if ((page = pgdir_alloc_page(mm->pgdir, la, perm)) == NULL) {
                goto bad_cleanup_mmap;
            }
            off = start - la, size = PAGE_SIZE - off, la += PAGE_SIZE;
            if (end < la) {
                size -= la - end;
            }
            memset(page2kva(page) + off, 0, size);
            start += size;
        }
    }

    //(4) build user stack memory
    // 需要给用户进程设置用户栈,为此调用 mm_mmap 函数建立用户栈的 vma struct,明确用户栈的位置在
    // 用户虚空间的顶端, 大小为 256 个 Page ,(256 * 4k = 1MB),并分配一定数量的物理内存且建立
    // 好栈的 va <~~~~> pa 映射关系.
    vm_flags = VM_READ | VM_WRITE | VM_STACK;
    if ((ret = mm_map(mm, USER_STACK_TOP - USER_STACK_SIZE, USER_STACK_SIZE, vm_flags, NULL)) != 0) {
        goto bad_cleanup_mmap;
    }
    assert(pgdir_alloc_page(mm->pgdir, USER_STACK_TOP-PAGE_SIZE , PTE_USER) != NULL);
    assert(pgdir_alloc_page(mm->pgdir, USER_STACK_TOP-2*PAGE_SIZE , PTE_USER) != NULL);
    assert(pgdir_alloc_page(mm->pgdir, USER_STACK_TOP-3*PAGE_SIZE , PTE_USER) != NULL);
    assert(pgdir_alloc_page(mm->pgdir, USER_STACK_TOP-4*PAGE_SIZE , PTE_USER) != NULL);

    //(5) set current process's mm, cr3, and set CR3 reg = physical addr of Page Directory
    // 至此,进程内的内存管理 vma 和 mm 已经构建完成,于是把 mm->pgdir 赋值 cr3 reg 中,即更新了
    // 用户进程的虚拟内存空间,此时的 initproc 已经被 hello 的代码和数据覆盖,成为了第一个用户进程,
    // 但此时这个用户进程的执行现场还没建立好;
    mm_count_inc(mm);
    current->mm = mm;
    current->cr3 = PADDR(mm->pgdir);
    lcr3(PADDR(mm->pgdir));

    //(6) setup trapframe for user environment
    // 先清空进程的中断帧,再重新设置进程的中断帧,使得在执行中断返回指令 “iret”后,能够让 CPU 转到
    // 用户态特权级, 并回到用户态内存空间,使用用户空间的代码段、数据段和 堆栈, 且能够跳转到用户进程
    // 的第一条指令执行,并确保在用户态能够响应中断;
    struct trapframe *tf = current->tf;
    memset(tf, 0, sizeof(struct trapframe));

    /* 
     * should set tf_cs,tf_ds,tf_es,tf_ss,tf_esp,tf_eip,tf_eflags
     * NOTICE: If we set trapframe correctly, then the user level process can return to USER MODE from kernel. So
     *          tf_cs should be USER_CS segment (see memlayout.h)
     *          tf_ds=tf_es=tf_ss should be USER_DS segment
     *          tf_esp should be the top addr of user stack (USER_STACK_TOP)
     *          tf_eip should be the entry point of this binary program (elf->e_entry)
     *          tf_eflags should be set to enable computer to produce Interrupt
     */
    tf->tf_cs = USER_CS;
    tf->tf_ds = tf->tf_es = tf->tf_ss = USER_DS;
    tf->tf_esp = USER_STACK_TOP;
    tf->tf_eip = elf->e_entry;
    tf->tf_eflags = FL_IF;
    ret = 0;
    // (7) 到这里,用户环境已经搭建完毕. 此时 initproc 将按产生系统调用的函数调用路径原路返回,
    //     执行中断返回指令 "iret"(trapentry.S 的最后一条)后，将切换到用户进程 hello 的第一条
    //     指令 _start 处(user/libs/initcode.S )开始执行.

out:
    return ret;
bad_cleanup_mmap:
    exit_mmap(mm);
bad_elf_cleanup_pgdir:
    put_pgdir(mm);
bad_pgdir_cleanup_mm:
    mm_destroy(mm);
bad_mm:
    goto out;
}

// do_execve - call exit_mmap(mm) & put_pgdir(mm) to reclaim memory space of current process
//           - call load_elf to setup new memory space accroding binary prog.
// 先回收自身所占用用户空间,然后调用 load_elf, 用新的程序覆盖内存空间,创建一个新进程.
// 首先为加载新的执行程序做好用户态内存空间清空准备. 如果 mm != NULL, 则设置 PT 为内核空间 PT,
// 且进一步判断 mm 的引用计数减 1 后是否为 0 , 如果为 0,则表明没有进程再需要此进程所占用的内存空间,
// 为此将根据 mm 中的记录,释放进程所占用户空间内存和进程 PT 本身所占空间。
// 最后把当前进程的 mm 内存管理指针设置为 NULL. 由于此处的 initproc 是内核线程,所以 mm=NULL，整个
// 处理都不用做.
int
do_execve(const char *name, size_t len, unsigned char *binary, size_t size) 
{
    cprintf("do_execve proc name [%s]\n",name);
    struct mm_struct *mm = current->mm;
    if (!user_mem_check(mm, (uintptr_t)name, len, 0)) {
        return -E_INVAL;
    }
    if (len > PROC_NAME_LEN) {
        len = PROC_NAME_LEN;
    }

    char local_name[PROC_NAME_LEN + 1];
    memset(local_name, 0, sizeof(local_name));
    memcpy(local_name, name, len);

    if (mm != NULL) {
        // 设置 PT 为内核空间 PT.
        lcr3(boot_cr3);
        // 判断是否还有其它进程需要此进程占用的空间. 
        if (mm_count_dec(mm) == 0) {
            exit_mmap(mm);
            put_pgdir(mm);
            mm_destroy(mm);
        }
        current->mm = NULL;
    }
    // load ELF
    int ret;
    if ((ret = load_elf(binary, size)) != 0) {
        goto execve_exit;
    }
    set_proc_name(current, local_name);
    return 0;

execve_exit:
    do_exit(ret);
    panic("already exit: %e.\n", ret);
}

// do_wait - wait one OR any children with PROC_ZOMBIE state, and free memory space of kernel stack
//         - proc struct of this child.
// NOTE: only after do_wait function, all resources of the child proces are free.
// 父进程等待子进程,并在得到子进程的退出消息后,彻底回收子进程所占的资源(比如子进程的内核栈和进程控制块).
int
do_wait(int pid, int *code_store) 
{
    struct mm_struct *mm = current->mm;
    if (code_store != NULL) {
        if (!user_mem_check(mm, (uintptr_t)code_store, sizeof(int), 1)) {
            return -E_INVAL;
        }
    }

    struct proc_struct *proc;
    bool intr_flag, haskid;
repeat:
    haskid = 0;
    if (pid != 0) {
        proc = find_proc(pid);
        if (proc != NULL && proc->parent == current) {
            haskid = 1;
            if (proc->state == PROC_ZOMBIE) {
                goto found;
            }
        }
    } else {
        proc = current->cptr;
        for (; proc != NULL; proc = proc->optr) {
            haskid = 1;
            if (proc->state == PROC_ZOMBIE) {
                goto found;
            }
        }
    }
    if (haskid) {
        current->state = PROC_SLEEPING;
        current->wait_state = WT_CHILD;
        schedule();
        if (current->flags & PF_EXITING) {
            do_exit(-E_KILLED);
        }
        goto repeat;
    }
    return -E_BAD_PROC;

found:
    if (proc == idleproc || proc == initproc) {
        panic("wait idleproc or initproc.\n");
    }
    if (code_store != NULL) {
        *code_store = proc->exit_code;
    }
    local_intr_save(intr_flag);
    {
        unhash_proc(proc);
        remove_links(proc);
    }
    local_intr_restore(intr_flag);
    put_kstack(proc);
    kfree(proc);
    return 0;
}

// do_kill - kill process with pid by set this process's flags with PF_EXITING
// 给一个进程设置 PF_EXITING 标志("kill" 信息,即要杀死它),这样 trap 中，将根据此标志,让进程退出.
int
do_kill(int pid) 
{
    struct proc_struct *proc;
    if ((proc = find_proc(pid)) != NULL) {
        if (!(proc->flags & PF_EXITING)) {
            proc->flags |= PF_EXITING;
            if (proc->wait_state & WT_INTERRUPTED) {
                wakeup_proc(proc);
            }
            return 0;
        }
        return -E_KILLED;
    }
    return -E_INVAL;
}

// kernel_execve - do SYS_exec syscall to exec a user program called by user_main kernel_thread
// vector128(0x80 vectors.S) --> __alltraps(trapentry.S)--> trap(trap.c)--> 
// trap_dispatch -->syscall(syscall.c)-->sys_exec(syscall.c)-->do_execve(proc.c)
// 最终通过 do_execve 函数完成用户进程的创建工作.

static int
kernel_execve(const char *name, unsigned char *binary, size_t size) 
{
    int ret, len = strlen(name);
    __asm__ volatile (
        "int %1;"
        : "=a" (ret)
        : "i" (T_SYSCALL), "0" (SYS_exec), "d" (name), "c" (len), "b" (binary), "D" (size)
        : "memory"
    );

    return ret;
}


#define __KERNEL_EXECVE(name, binary, size) ({                          \
            cprintf("kernel_execve: pid = %d, name = \"%s\".\n",        \
                    current->pid, name);                                \
            kernel_execve(name, binary, (size_t)(size));                \
        })

#define KERNEL_EXECVE(x) ({                                             \
            extern unsigned char _binary_obj___user_##x##_out_start[],  \
                _binary_obj___user_##x##_out_size[];                    \
            __KERNEL_EXECVE(#x, _binary_obj___user_##x##_out_start,     \
                            _binary_obj___user_##x##_out_size);         \
        })

#define __KERNEL_EXECVE2(x, xstart, xsize) ({                           \
            extern unsigned char xstart[], xsize[];                     \
            __KERNEL_EXECVE(#x, xstart, (size_t)xsize);                 \
        })

#define KERNEL_EXECVE2(x, xstart, xsize)        __KERNEL_EXECVE2(x, xstart, xsize)

// user_main - kernel thread used to exec a user program
static int
user_main(void *arg) 
{
#ifdef TEST
    cprintf("user TEST base !!!\n");
    KERNEL_EXECVE2(TEST, TESTSTART, TESTSIZE);
#else
    cprintf("user main base !!!\n");
    KERNEL_EXECVE(exit);
#endif
    panic("user_main execve failed.\n");
}


// init_main - the second kernel thread used to create user_main kernel threads
static int
init_main(void *arg) 
{
    size_t nr_free_pages_store = nr_free_pages();
    size_t kernel_allocated_store = kallocated();

    int pid = kernel_thread(user_main,NULL,0);
    if (pid <= 0) {
        panic("create user_main failed. \n");
    }
    
    while (do_wait(0,NULL) == 0) {
        schedule();
    }
    cprintf("all user-mode processes have quit.\n");
    assert(initproc->cptr == NULL && initproc->yptr == NULL && initproc->optr == NULL);
    assert(nr_process == 2);
    assert(list_next(&proc_list) == &(initproc->list_link));
    assert(list_prev(&proc_list) == &(initproc->list_link));
    assert(nr_free_pages_store == nr_free_pages());
    assert(kernel_allocated_store == kallocated());
    cprintf("init check memory pass.\n");
    return 0;
}

// proc_init - set up the first kernel thread idleproc "idle" by itself and 
//           - create the second kernel thread init_main
void
proc_init(void) 
{
    int i;

    list_init(&proc_list);
    for (i = 0; i < HASH_LIST_SIZE; i ++) {
        list_init(hash_list + i);
    }

    if ((idleproc = alloc_proc()) == NULL) {
        panic("cannot alloc idleproc.\n");
    }

    idleproc->pid = 0;
    idleproc->state = PROC_RUNNABLE;
    idleproc->kstack = (uintptr_t)bootstack;
    idleproc->need_resched = 1;
    set_proc_name(idleproc, "idle");
    nr_process ++;

    current = idleproc;

    int pid = kernel_thread(init_main, NULL, 0);
    if (pid <= 0) {
        panic("create init_main failed.\n");
    }

    initproc = find_proc(pid);
    set_proc_name(initproc, "init");

    assert(idleproc != NULL && idleproc->pid == 0);
    assert(initproc != NULL && initproc->pid == 1);
}

// cpu_idle - at the end of kern_init, the first kernel thread idleproc will do below works
void
cpu_idle(void) 
{
    while (1) {
        if (current->need_resched) {
            schedule();
        }
    }
}

//FOR proj6, set the process's priority (bigger value will get more CPU time) 
void
proj6_set_priority(uint32_t priority) 
{
    if (priority == 0) {
        current->proj6_priority = 1;
    } else {
        current->proj6_priority = priority;
    }
}
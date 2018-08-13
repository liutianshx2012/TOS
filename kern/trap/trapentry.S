#include <memlayout.h>

# 紧接着第一步处理后,进一步完成第二步处理;
# 并且有恢复中断上下文的处理,即中断处理完毕后的返回准备工作;
# 中断可分为两类 1==> push error code ; 2==> push 0; push 中断ID. 共push 两个参数,
# 中断处理函数 跳转到 统一的入口 __alltraps 处.
# vectors.S sends all traps here.
.text
.globl __alltraps
__alltraps:
    # push registers to build a trap frame
    # therefore make the stack look like a struct trapframe
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs
    pushal

    # load G_D_K_DATA into %ds and %es to set up data segments for kernel
    movl $G_D_K_DATA, %eax
    movw %ax, %ds
    movw %ax, %es

    # push %esp to pass a pointer to the trapframe as an argument to trap()
    pushl %esp

    # call trap(tf), where tf=%esp
    call trap  # into C 

    # pop the pushed stack pointer
    popl %esp

    # return falls through to trapret...
.globl __trapret
__trapret:
    # restore registers from stack
    popal

    # restore %ds, %es, %fs and %gs
    popl %gs
    popl %fs
    popl %es
    popl %ds

    # get rid of the trap number and error code
    addl $0x8, %esp
    iret

.globl forkrets
forkrets:
    # set stack to this new process's trapframe
    # 把 esp 指向当前进程的中断帧, 从 __trapret 开始执行到 iret 前, esp
    # 指向了 current->tf.tf_eip, 而如果此时执行的是 initproc则
    # current->tf.tf_eip = kernel_thread_entry, initproc->tf.tf_cs = KERNEL_CS, 所以当执# 行完 iret 后,就开始在内核中执行 kernel_thread_entry 函数了.
    # 而 initproc->tf.tf_regs.reg_ebx = init_main,  所以在 kernl_thread_entry 中执行 
    # “call %ebx” 后,就开始执行 initproc 的主体了.
    
    movl 4(%esp), %esp
    jmp __trapret


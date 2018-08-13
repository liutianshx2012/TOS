# 上下文切换,利用堆栈保存 | 恢复进程上下文信息

.text
.globl switch_to
switch_to:                      # switch_to(from, to)

    # save from's registers 
    # 前两条指令,保存了进程在返回 switch_to 函数后的指令地址到 context.eip 中.
    movl 4(%esp), %eax          # eax points to from
    popl 0(%eax)        # esp--> return address, so save return addr in FROM’s context
    
    # 接下来的 7 条汇编指令完成了保存 from 进程的其它 7 个 regs 到context 中的相应成员变量中。 
    movl %esp, 4(%eax)
    movl %ebx, 8(%eax)
    movl %ecx, 12(%eax)
    movl %edx, 16(%eax)
    movl %esi, 20(%eax)
    movl %edi, 24(%eax)
    movl %ebp, 28(%eax)
    # 到这里 from 线程的执行现场保存完毕.  

    # 下面要开始恢复 to 线程的执行现场,其实是上面保存过程的 逆操作.
    # restore to's registers
    movl 4(%esp), %eax          # not 8(%esp): popped return address already
    
    # eax now points to to
    # 从 context 的高地址的成员变量 ebp 开始,逐一把相关成员变量的值赋值给对应的寄存器
    movl 28(%eax), %ebp
    movl 24(%eax), %edi
    movl 20(%eax), %esi
    movl 16(%eax), %edx
    movl 12(%eax), %ecx
    movl 8(%eax), %ebx
    movl 4(%eax), %esp

    # 把 context 中保存的下一个进程要执行的指令地址 context.eip 放到了堆栈顶
# push TO’s context’s eip, so return addr = TO’s eip
    pushl 0(%eax)       

    ret                      # after ret, eip= TO’s eip


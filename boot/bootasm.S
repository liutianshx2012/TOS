#include <asm.h>

# Start the CPU: switch to 32-bit protected mode, jump into C.
# The BIOS loads this code from the first sector of the hard disk into
# memory at physical address 0x7c00 and starts executing in real mode
# with %cs=0 %ip=7c00.
# Bootload的启动过程可以概括如下:
# 1=> 首先,BIOS从主加载分区中物理地址为0x7c00的位置加载以下代码并开始执行实模式代码,段寄存器 cs=0,ip=0x7c00;
# 2=> CLI屏蔽中断;CLD使DF复位,即DF=0,串操作方向控制;
# 3=> 设置寄存器 ax,ds,es,ss寄存器值清0;地址线20被封锁,高于1MB的地址都默认回卷到0.
#     怎么激活A20呢?由于历史原因A20地址位由键盘控制器芯片8042管理.所以要给8042发命令激活A20;
#     8042有两个IO端口:0x60和0x64, 激活流程位: 发送0xd1命令到0x64端口 --> 发送0xdf到0x60.
# 4=> 从实模式转换到保护模式,用到了全局描述符表和段表,使得虚拟地址和物理地址匹配,保证转换时有效的内存映射不改变;
#     LGDT汇编指令把GDT描述符表的大小和起始位置存入GDTR寄存器中;将CR0的最后一位设置为1,进入保护模式;
#     指令跳转由代码段跳到protcseg的起始位置.
# 5=> 设置保护模式下数据段寄存器;设置堆栈寄存器并调用main函数;对GDT作处理.
.set KERNEL_CS,        0x8                     # kernel code segment selector
.set KERNEL_DS,        0x10                    # kernel data segment selector
.set CR0_PE_ON,        0x1                     # protected mode enable flag

# start address should be 0:7c00, in real mode, the beginning address of the running bootloader
.globl start
start:
.code16                                             # Assemble for 16-bit mode
    cli                                             # Disable interrupts  (在16位下)关闭中断
    cld                                             # String operations increment 设置字符串操作是递增方向

    # Set up the important data segment registers (DS, ES, SS). 设置重要数据段计数器(DS, ES, SS).
    xorw %ax, %ax                                   # Segment number zero  eax寄存器(low 16位)设置为0
    movw %ax, %ds                                   # -> Data Segment    数据段寄存器
    movw %ax, %es                                   # -> Extra Segment   扩展段寄存器
    movw %ax, %ss                                   # -> Stack Segment   栈段寄存器

    # Enable A20:  将A20使能(打开A20地址线)
    #  For backwards compatibility with the earliest PCs, physical
    #  address line 20 is tied low, so that addresses higher than
    #  1MB wrap around to zero by default. This code undoes this.
    # 为了兼容早期的PC机,第20根地址线在实模式下不能使用
    # 所以超过1MB的地址,默认就会返回到地址0,重新从0循环计数
    # 下面的代码打开A20地址线
seta20.1:
    inb $0x64, %al   # Wait for not busy(8042 input buffer empty). 从0x64端口读入一个字节的数据到al中 (al寄存器为eax寄存器的低8位)
    testb $0x2, %al  # test指令可以当作and指令,只不过它不会影响操作数
    jnz seta20.1     # 如果上面的测试中发现al的第2位为0(代表键盘缓冲区为空),就不执行该指令;否则就循环检查

    movb $0xd1, %al                                 # 0xd1 -> port 0x64
    outb %al, $0x64                                 # 0xd1 将al中的数据写入到端口0x64中

seta20.2:
    inb $0x64, %al    # Wait for not busy(8042 input buffer empty). 从0x64端口读取一个字节的数据到al中
    testb $0x2, %al   # 测试al的第2位是否为0
    jnz seta20.2      # 如果上面的测试中发现al的第2位为0,就不执行该指令  否则就循环检查

    movb $0xdf, %al  # 0xdf -> port 0x60 打开A20
    outb %al, $0x60  # 0xdf = 11011111, means set P2's A20 bit(the 1 bit) to 1  ; 将al中的数据写入到0x60端口中

    # Switch from real to protected mode, using a bootstrap GDT
    # and segment translation that makes virtual addresses
    # identical to physical addresses, so that the
    # effective memory map does not change during the switch.
    # cr0中的第0位为1表示处于保护模式
    # cr0中的第0位为0,表示处于实模式
    # 初始化GDT表: 一个简单的GDT表和其描述符已经静态储存在引导区中
    LGDT GDT_DESC        # 将全局描述符表描述符加载到全局描述符表寄存器
    # 进入保护模式: 通过将cr0寄存器PE位置1便开启了保护模式
    movl %cr0, %eax      # 把控制寄存器cr0加载到eax中
    orl $CR0_PE_ON, %eax # 将eax中的第0位设置为1
    movl %eax, %cr0      # 将eax中的值装入cr0中

    # Jump to next instruction, but in 32-bit code segment. 跳转到32位模式中的下一条指令
    # Switches processor into 32-bit mode.   将处理器切换为32位工作模式
    # 下面这条指令执行的结果会将$KERNEL_CS加载到cs中,cs对应的高速缓冲存储器会加载代码段描述符,同样将$protcseg加载到ip中
    # 通过长跳转更新 cs 的基地址
    ljmp $KERNEL_CS, $protcseg   # KERNEL_CS=0x8

.code32                                             # Assemble for 32-bit mode
protcseg:
    # Set up the protected-mode data segment registers
    # 设置段寄存器,并建立堆栈
    movw $KERNEL_DS, %ax                            # Our data segment selector
    movw %ax, %ds                                   # -> DS: Data Segment
    movw %ax, %es                                   # -> ES: Extra Segment
    movw %ax, %fs                                   # -> FS
    movw %ax, %gs                                   # -> GS
    movw %ax, %ss                                   # -> SS: Stack Segment

    # Set up the stack pointer and call into C. The stack region is from 0--start(0x7c00)
    movl $0x0, %ebp
    movl $start, %esp
    call bootmain                    # In    C

    # If bootmain returns (it shouldn't), loop.
    spin:
        jmp spin

# Bootstrap GDT
.p2align 2                                          # force 4 byte alignment
GDT:
    SEG_NULLASM                                     # null seg    
    SEG_ASM(STA_X|STA_R, 0x0, 0xffffFFFF)           # code seg descriptor for bootloader and kernel 代码段描述符  STA_X (0x8)|STA_R(0x2) = 1000 | 0010 = 1010 => 0xa 
    SEG_ASM(STA_W, 0x0, 0xFFFFffff)                 # data seg for bootloader and kernel  数据段描述符

GDT_DESC:
    .word 0x17                                      # sizeof(GDT) - 1
    .long GDT                                       # address GDT

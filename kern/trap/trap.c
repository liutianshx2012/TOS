/*************************************************************************
	> File Name: trap.c
	> Author:TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 13时04分30秒
 ************************************************************************/
#include <defs.h>
#include <mmu.h>
#include <memlayout.h>
#include <clock.h>
#include <x86.h>
#include <stdio.h>
#include <assert.h>
#include <console.h>
#include <kdebug.h>
#include <string.h>
#include <trap.h>

/* 初步处理后,继续完成具体的各种中断处理操作. */

#define TICK_NUM 100

static void 
print_ticks(void)
{
    cprintf("time trap ===> %d ticks\n",TICK_NUM);
}

/* *
 * Interrupt descriptor table:
 *
 * Must be built at run time because shifted function addresses can't
 * be represented in relocation records.
 * */

static struct gatedesc idt[256] = {{0}};

static struct pseudodesc idt_pd =
{
    sizeof(idt)-1,(uintptr_t)idt
};
/* 这里主要就是实现对中断向量表的初始化. Interrupt  Decriptor Table .*/
void
idt_init(void)
{
    /*  step 2*
    * (1) Where are the entry addrs of each Interrupt Service Routine(ISR)?
    *     All ISR's entry addrs are stored in __vectors.where is uintptr_t *
    *     __vectors[]?
    *     __vectors[] is in kern/trap/vector.S which is produced by 
    *     tools/vector.c (try "make" command ini proj1,then you will find *
    *     vector.S in kern/trap DIR )
    *     You can use "extern uintptr_t __vectors[];" to define this extern 
    *     variable which will be used later.
    * 
    * (2) Now you should setup the entries of ISR in Interrupt Descriptor 
    *     Table(IDT).
    *     can you ses idt[256] in this File? Yes, it's IDT! you can use *
    *     SETGATE macro to setup each item of IDT.
    *     
    * (3) After setup the contents of IDT,You will let CPU know Where is 
    *     the IDT by using 'lidt' instruction.
    *     You don't know the meaning of this instruction? just goole it! 
    *     and check the libs/x86.h to know more.
    *     Notice: the argument of lidt is idt_pd. try to find it !
    **/
    /**
     * 第一步,声明vertors,其中存放着中断服务程序的入口地址.这个数组生成于vertor.S中.
     * 第二步,填充中断描述符表IDT.
     * 第三步,加载中断描述符表.
     */
    extern uintptr_t __vectors[];
    int i;
    for (i = 0; i < sizeof(idt) / sizeof(struct gatedesc); i++) {
        SETGATE(idt[i],0,G_D_K_TEXT, __vectors[i],DPL_KERNEL);
    }
    /* SET for switch from user to kernel*/
    SETGATE(idt[USER_SWITCH_2_KERNEL], 0, KERNEL_CS, __vectors
                              [USER_SWITCH_2_KERNEL], DPL_KERNEL);

    /*load the IDT ==> 使用 LIDT 指令加载 IDT 中断描述符表*/
    lidt(&idt_pd);
}

static const char *
trapname(int trapno)
{
    static const char * const excnames[] =
    {
        "Divide error",
        "Debug",
        "Non-Maskable Interrupt",
        "Breakpoint",
        "Overflow",
        "BOUND Range Exceeded",
        "Invalid Opcode",
        "Device Not Available",
        "Double Fault",
        "Coprocessor Segment Overrun",
        "Invalid TSS",
        "Segment Not Present",
        "Stack Fault",
        "General Protection",
        "Page Fault",
        "(unknown trap)",
        "x87 FPU Floating-Point Error",
        "Alignment Check",
        "Machine-Check",
        "SIMD Floating-Point Exception"
    };

    if (trapno < sizeof(excnames)/sizeof(const char * const)) {
        return excnames[trapno];
    }
    if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + 16) {
        return "Hardware Interrupt";
    }
    return "(unknown trap)";
}

/* trap_in_kernel - test if trap happened in kernel */
bool
trap_in_kernel(struct trapframe *tf)
{
    return (tf->tf_cs == (uint16_t)KERNEL_CS);
}

static const char *IA32flags[] =
{
    "CF", NULL, "PF", NULL, "AF", NULL, "ZF", "SF",
    "TF", "IF", "DF", "OF", NULL, NULL, "NT", NULL,
    "RF", "VM", "AC", "VIF", "VIP", "ID", NULL, NULL,
};

void
print_trapframe(struct trapframe *tf)
{
    cprintf("trapframe at %p\n", tf);
    
    print_regs(&tf->tf_regs);

    cprintf("  ds   0x----%04x\n", tf->tf_ds);
    cprintf("  es   0x----%04x\n", tf->tf_es);
    cprintf("  fs   0x----%04x\n", tf->tf_fs);
    cprintf("  gs   0x----%04x\n", tf->tf_gs);
    cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
    cprintf("  err  0x%08x\n", tf->tf_err);
    cprintf("  eip  0x%08x\n", tf->tf_eip);
    cprintf("  cs   0x----%04x\n", tf->tf_cs);
    cprintf("  flag 0x%08x ", tf->tf_eflags);

    int i, j;
    for (i = 0, j = 1; i < sizeof(IA32flags) / sizeof(IA32flags[0]); i ++, j <<= 1) {
        if ((tf->tf_eflags & j) && IA32flags[i] != NULL) {
            cprintf("%s,", IA32flags[i]);
        }
    }
    cprintf("IOPL====> %d\n", (tf->tf_eflags & FL_IOPL_MASK) >> 12);

    if (!trap_in_kernel(tf)) {
        cprintf("  esp  0x%08x\n", tf->tf_esp);
        cprintf("  ss   0x----%04x\n", tf->tf_ss);
    }
}

void
print_regs(struct pushregs *regs)
{
    cprintf("  edi  0x%08x\n", regs->reg_edi);
    cprintf("  esi  0x%08x\n", regs->reg_esi);
    cprintf("  ebp  0x%08x\n", regs->reg_ebp);
    cprintf("  oesp 0x%08x\n", regs->reg_oesp);
    cprintf("  ebx  0x%08x\n", regs->reg_ebx);
    cprintf("  edx  0x%08x\n", regs->reg_edx);
    cprintf("  ecx  0x%08x\n", regs->reg_ecx);
    cprintf("  eax  0x%08x\n", regs->reg_eax);
}

/* temporary trapframe or pointer to trapframe */
struct trapframe switchk2u, *switchu2k;

/* trap_dispatch - dispatch based on what type of trap occurred */
static void
trap_dispatch(struct trapframe *tf)
{
    char c;

    switch (tf->tf_trapno) {
        case IRQ_OFFSET + IRQ_TIMER:  //时钟中断
            /* STEP 3 */
            /* handle the timer interrupt */
            /* (1) After a timer interrupt, you should record this event using a global variable (increase it), such as ticks in kern/driver/clock.c
            * (2) Every TICK_NUM cycle, you can print some info using a funciton, such as print_ticks().
            * (3) Too Simple? Yes, I think so!
            */
            ticks ++;
            if (ticks % TICK_NUM == 0) {
                print_ticks();
            }
            break;
        case IRQ_OFFSET + IRQ_COM1://串口中断,显示收到的字符
            c = cons_getc();
            cprintf("serial [%03d] %c\n", c, c);
            break;
        case IRQ_OFFSET + IRQ_KBD: //键盘中断
            c = cons_getc();
            cprintf("键盘中断 kbd [%03d] %c\n", c, c);
            break;
            //proj1 CHALLENGE 1 : YOUR CODE you should modify below codes.
        case KERNEL_SWITCH_2_USER:
            if (tf->tf_cs != USER_CS) {
                switchk2u = *tf;
                switchk2u.tf_cs = USER_CS;
                switchk2u.tf_ds = switchk2u.tf_es = switchk2u.tf_ss = USER_DS;
                switchk2u.tf_esp = (uint32_t)tf + sizeof(struct trapframe) - 8;

                // set eflags, make sure trap can use io under user mode.
                // if CPL > IOPL, then cpu will generate a general protection.
                switchk2u.tf_eflags |= FL_IOPL_MASK;

                // set temporary stack
                // then iret will jump to the right stack
                *((uint32_t *)tf - 1) = (uint32_t)&switchk2u;
            }
            break;
        case USER_SWITCH_2_KERNEL:
            // cprintf("trap dispatch USER_SWITCH_2_KERNEL\n");
            // print_trapframe(tf);
            if (tf->tf_cs != KERNEL_CS) {
                tf->tf_cs = KERNEL_CS;
                tf->tf_ds = tf->tf_es = KERNEL_DS;
                tf->tf_eflags &= ~FL_IOPL_MASK;
                switchu2k = (struct trapframe *)(tf->tf_esp - (sizeof(struct trapframe) - 8));
                memmove(switchu2k, tf, sizeof(struct trapframe) - 8);
                *((uint32_t *)tf - 1) = (uint32_t)switchu2k;
            }
            break;
        case IRQ_OFFSET + IRQ_IDE1:
        case IRQ_OFFSET + IRQ_IDE2:
            /* do nothing */
            break;
        default:
            // in kernel, it must be a mistake
            if ((tf->tf_cs & 3) == 0) {
                print_trapframe(tf);
                panic("unexpected trap in kernel.\n");
            }
        }
}

/* *trap -
 * handles or dispatches an exception/interrupt. if and when trap() returns,
 * the code in kern/trap/trapentry.S restores the old CPU state saved in the
 * trapframe and then uses the iret instruction to return from the exception.
 * */
void
trap(struct trapframe *tf)
{
    // dispatch based on what type of trap occurred
    trap_dispatch(tf);
}

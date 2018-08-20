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
#include <vmm.h>
#include <swap.h>
#include <unistd.h>
#include <syscall.h>
#include <error.h>
#include <sched.h>
#include <sync.h>
#include <proc.h>
#include <trap.h>

/* 初步处理后,继续完成具体的各种中断处理操作. */

#define TICK_NUM    100

static volatile int in_swap_tick_event = 0;
/* temporary trapframe or pointer to trapframe */
extern struct mm_struct *check_mm_struct;


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
    // 设置用于系统调用的中断门信息  int 0x80;
    SETGATE(idt[T_SYSCALL], 1, G_D_K_TEXT, __vectors[T_SYSCALL], DPL_USER);

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
    cprintf("IOPL=%d\n", (tf->tf_eflags & FL_IOPL_MASK) >> 12);

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

static inline void
print_pgfault(struct trapframe *tf) 
{
    /* error_code:
     * bit 0 == 0 means no page found, 1 means protection fault
     * bit 1 == 0 means read, 1 means write
     * bit 2 == 0 means kernel, 1 means user
     * */
    cprintf("page fault at 0x%08x: %c/%c [%s].\n", rcr2(),
            (tf->tf_err & 4) ? 'U' : 'K',
            (tf->tf_err & 2) ? 'W' : 'R',
            (tf->tf_err & 1) ? "protection fault" : "no page found");
}

static int 
pgfault_handler(struct trapframe *tf) 
{
    cprintf("pgfault_handler tf_err : [0x%08x].\n", tf->tf_err);

    //used for test check_swap
    if(check_mm_struct !=NULL) { 
        print_pgfault(tf);
    }

    struct mm_struct *mm;
    if (check_mm_struct != NULL) {
        assert(current == idleproc);
        mm = check_mm_struct;
    } else {
        if (current == NULL) {
            print_trapframe(tf);
            print_pgfault(tf);
            panic("unhandled page fault.\n");
        }
        mm = current->mm;
    }
    //cprintf("mm =>[0x%08lx]\n",mm);
    return do_pgfault(mm, tf->tf_err, rcr2());
}

/* trap_dispatch - dispatch based on what type of trap occurred */
static void
trap_dispatch(struct trapframe *tf)
{
    char c;
    int ret = 0;
    switch (tf->tf_trapno) {
        case T_PGFLT : { //page fault
            if ((ret = pgfault_handler(tf)) != 0) {
                print_trapframe(tf);
                if (current == NULL) {
                    panic("handle pgfault failed. ret=%d\n", ret);
                } else {
                    if (trap_in_kernel(tf)) {
                        panic("handle pgfault failed in kernel mode. ret=%d\n", ret);
                    }
                    cprintf("killed by kernel.\n");
                    panic("handle user mode pgfault failed. ret=%d\n", ret); 
                    do_exit(-E_KILLED);
                }
            }
            break;
        }
        case T_SYSCALL : {
            syscall();
            break;
        }
        case IRQ_OFFSET + IRQ_TIMER: {  //时钟中断
            ticks ++;
            assert(current != NULL);
            print_ticks();
			run_timer_list(); //proj 7
            break;
        }
        case IRQ_OFFSET + IRQ_COM1: { //串口中断,显示收到的字符
            c = cons_getc();
            cprintf("serial [%03d] %c\n", c, c);
            break;
        }
        case IRQ_OFFSET + IRQ_KBD: { //键盘中断
            c = cons_getc();
            cprintf("键盘中断 kbd [%03d] %c\n", c, c);
            break;
        }
        case KERNEL_SWITCH_2_USER:
        case USER_SWITCH_2_KERNEL: 
            panic("_SWITCH_** ??\n");
            break;
        case IRQ_OFFSET + IRQ_IDE1:
        case IRQ_OFFSET + IRQ_IDE2: {
            /* do nothing */
            break;
        }
        default: {
            print_trapframe(tf);
            if (current != NULL) {
                cprintf("unhandled trap.\n");
                do_exit(-E_KILLED);
            }
            // in kernel, it must be a mistake
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
    // used for previous projects
    if (current == NULL) {
        trap_dispatch(tf);
    } else {
        // keep a trapframe chain in stack
        struct trapframe *otf = current->tf;
        current->tf = tf;
    
        bool in_kernel = trap_in_kernel(tf);
    
        trap_dispatch(tf);
    
        current->tf = otf;
        if (!in_kernel) {
            if (current->flags & PF_EXITING) {
                do_exit(-E_KILLED);
            }
            if (current->need_resched) {
                schedule();
            }
        }
    }
}

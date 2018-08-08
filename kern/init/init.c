/*************************************************************************
	> File Name: init/init.c
	> Author:TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 10时51分07秒
 ************************************************************************/
#include <defs.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <kdebug.h>
#include <picirq.h>
#include <trap.h>
#include <clock.h>
#include <intr.h>
#include <pmm.h>
#include <vmm.h>
#include <ide.h>
#include <swap.h>
#include <kmonitor.h>

void kern_init(void) __attribute__((noreturn));

static void test_switch_kernel_user_model(void);

void
kern_init(void)
{
    extern char edata[],end[];
    memset(edata,0,end-edata);

    cons_init();
    const char *message = "TOS is loading...";
    cprintf("%s\n\n",message);
    print_kerninfo();
    mon_backtrace(0, NULL, NULL);

    pmm_init();                 // init physical memory management

    pic_init();                 // init interrupt controller
    idt_init();                 // init interrupt descriptor table

    vmm_init();                 // init virtual memory management
    ide_init();                 // init ide devices
    swap_init();                // init swap

    clock_init();               // init clock interrupt
    intr_enable();              // enable irq interrupt

    // user/kernel mode switch test
//    test_switch_kernel_user_model();

    /*do nothing*/
    while(1);
}


static void
print_cur_status(void)
{
    static int round = 0;
    uint16_t reg1, reg2, reg3, reg4;
    __asm__ volatile (
        "mov %%cs, %0;"
        "mov %%ds, %1;"
        "mov %%es, %2;"
        "mov %%ss, %3;"
        : "=m"(reg1), "=m"(reg2), "=m"(reg3), "=m"(reg4));
    cprintf("%d: @ring %d\n", round, reg1 & 3);
    cprintf("%d:  cs = %x\n", round, reg1);
    cprintf("%d:  ds = %x\n", round, reg2);
    cprintf("%d:  es = %x\n", round, reg3);
    cprintf("%d:  ss = %x\n", round, reg4);
    round ++;
}

static void
test_switch_to_user(void)
{
    __asm__ volatile (
        "sub $0x8, %%esp;"
        "int %0;"
        "movl %%ebp, %%esp"
        :
        : "i"(KERNEL_SWITCH_2_USER)
    );
    cprintf("trap ret test_switch_to_user....\n");
}

static void
test_switch_to_kernel(void)
{
    __asm__ volatile (
        "int %0;"
        "movl %%ebp, %%esp;"
        :
        : "i"(USER_SWITCH_2_KERNEL)
    );
    cprintf("trap ret test_switch_to_kernel....\n");
}

static void
test_switch_kernel_user_model(void)
{
    print_cur_status();
    cprintf("+++ switch to  user  mode +++\n");
    test_switch_to_user();
    print_cur_status();
    cprintf("+++ switch to kernel mode +++\n");
    test_switch_to_kernel();
    print_cur_status();
}

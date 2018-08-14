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
#include <proc.h>
#include <kmonitor.h>

void kern_init(void) __attribute__((noreturn));

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
  	proc_init(); 				        // init process table
    ide_init();                 // init ide devices
    swap_init();                // init swap

    clock_init();               // init clock interrupt
    intr_enable();              // enable irq interrupt

    cpu_idle();					// run idle process
}


// static void
// print_cur_status(void) 
// {
//     static int round = 0;
//     uint16_t reg1, reg2, reg3, reg4;
//     __asm__ volatile (
//         "mov %%cs, %0;"
//         "mov %%ds, %1;"
//         "mov %%es, %2;"
//         "mov %%ss, %3;"
//         : "=m"(reg1), "=m"(reg2), "=m"(reg3), "=m"(reg4));
//     cprintf("%d: @ring %d\n", round, reg1 & 3);
//     cprintf("%d:  cs = %x\n", round, reg1);
//     cprintf("%d:  ds = %x\n", round, reg2);
//     cprintf("%d:  es = %x\n", round, reg3);
//     cprintf("%d:  ss = %x\n", round, reg4);
//     round ++;
// }


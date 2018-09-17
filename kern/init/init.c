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
    // mon_backtrace(0, NULL, NULL);

    pmm_init();

    pic_init();
    idt_init(); 

    clock_init();
    intr_enable();

    /*do nothing*/
    while(1);
}



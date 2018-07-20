/*************************************************************************
	> File Name: intr.c
	> Author:TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 14时28分54秒
 ************************************************************************/
#include <x86.h>
#include <intr.h>
/*	实现了通过设置CPU的flags来屏蔽和使能中断的函数 */


/* intr_enable - enable irq interrupt */
void
intr_enable(void)
{
    sti();
}

/* intr_disable - disable irq interrupt */
void
intr_disable(void)
{
    cli();
}

/*************************************************************************
	> File Name: picirq.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 14时30分40秒
 ************************************************************************/
#ifndef _KERN_DRIVER_PICIRQ_H
#define _KERN_DRIVER_PICIRQ_H

void pic_init(void);

void pic_enable(unsigned int irq);

#define IRQ_OFFSET      32

#endif

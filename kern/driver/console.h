/*************************************************************************
	> File Name: console.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 14时24分04秒
 ************************************************************************/
#ifndef __KERN_DRIVER_CONSOLE_H
#define __KERN_DRIVER_CONSOLE_H

void cons_init(void);

void cons_putc(int c);

int cons_getc(void);

void serial_intr(void);

void kbd_intr(void);

#endif

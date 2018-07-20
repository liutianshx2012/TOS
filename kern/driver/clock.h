/*************************************************************************
	> File Name: clock.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 14时19分08秒
 ************************************************************************/
#ifndef _KERN_DRIVER_CLOCK_H
#define _KERN_DRIVER_CLOCK_H

#include <defs.h>

extern volatile size_t ticks;

void clock_init(void);

#endif

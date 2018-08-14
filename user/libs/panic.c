/*************************************************************************
	> File Name: panic.c
	> Author: TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Mon 01 Aug 2016 08:53:14 PM PDT
 ************************************************************************/
#include <defs.h>
#include <stdarg.h>
#include <stdio.h>
#include <ulib.h>
#include <error.h>

void
__panic(const char *file, int line ,const char *fmt,...) 
{
    // print the 'message'
    va_list ap;
    va_start(ap,fmt);
    cprintf("user panic at %s:%d: \n",file,line);
    vcprintf(fmt,ap);
    cprintf("\n");
    va_end(ap);
    exit(-E_PANIC);
}

void
__warn(const char *file, int line ,const char *fmt,...) 
{
    va_list ap;
    va_start(ap,fmt);
    cprintf("user warning at %s : %d",file,line);
    vcprintf(fmt,ap);
    cprintf("\n");
    va_end(ap);
}


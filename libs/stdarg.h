/*************************************************************************
	> File Name: stdarg.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 10时07分27秒
 ************************************************************************/
#ifndef _LIBS_STDARG_H
#define _LIBS_STDARG_H

/* compiler provides size of save area */
typedef __builtin_va_list va_list;

#define va_start(ap, last)              (__builtin_va_start(ap, last))
#define va_arg(ap, type)                (__builtin_va_arg(ap, type))
#define va_end(ap)                      /*nothing*/



#endif

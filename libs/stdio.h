/*************************************************************************
	> File Name: stdio.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 10时09分00秒
 ************************************************************************/
#ifndef _LIBS_STDIO_H
#define _LIBS_STDIO_H

#include <defs.h>
#include <stdarg.h>

/* kern/libs/stdio.c */
int cprintf(const char *fmt, ...);

int vcprintf(const char *fmt, va_list ap);

void cputchar(int c);

int cputs(const char *str);

int getchar(void);

/* kern/libs/readline.c */
char *readline(const char *prompt);

/* libs/printfmt.c */
void printfmt(void (*putch)(int, void *), void *putdat, const char *fmt, ...);

void vprintfmt(void (*putch)(int, void *), void *putdat, const char *fmt, va_list ap);

int snprintf(char *str, size_t size, const char *fmt, ...);

int vsnprintf(char *str, size_t size, const char *fmt, va_list ap);


#endif

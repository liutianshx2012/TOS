/*************************************************************************
	> File Name: ulib.h
	> Author: Ttc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Mon 01 Aug 2016 07:50:43 PM PDT
 ************************************************************************/
#ifndef _USER_LIBS_ULIB_H
#define _USER_LIBS_ULIB_H

#include <defs.h>

void __warn(const char *file, int line, const char *fmt, ...);
void __noreturn __panic(const char *file, int line, const char *fmt, ...);

#define warn(...)                                       \
    __warn(__FILE__, __LINE__, __VA_ARGS__)

#define panic(...)                                      \
    __panic(__FILE__, __LINE__, __VA_ARGS__)

#define assert(x)                                       \
    do {                                                \
        if (!(x)) {                                     \
            panic("assertion failed: %s", #x);          \
        }                                               \
    } while (0)

// static_assert(x) will generate a compile-time error if 'x' is false.
#define static_assert(x)                                \
    switch (x) { case 0: case (x): ; }

void __noreturn exit(int error_code);

int fork(void);

int wait(void);

int waitpid(int pid, int *store);

void yield(void);

int kill(int pid);

int getpid(void);

void print_pgdir(void);


int tprintf(const char *fmt, ...); 
#endif

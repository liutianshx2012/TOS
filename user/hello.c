/*************************************************************************
	> File Name: hello.c
	> Author: TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Mon 01 Aug 2016 11:13:31 PM PDT
 ************************************************************************/
#include <stdio.h>
#include <ulib.h>

int
main(void) 
{
    cprintf("TTc hello world!!!.\n");
    cprintf("I am procces 2 %d.\n",getpid());
    cprintf("hello end.\n TTc know how to display 'hello world'\n");
    return 0;
}

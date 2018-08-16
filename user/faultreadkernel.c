/*************************************************************************
	> File Name: faultreadkernel.c
	> Author: TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Sun 14 Aug 2016 01:55:04 AM PDT
 ************************************************************************/
#include <stdio.h>
#include <ulib.h>

int
main(void) 
{
    cprintf("I read %08x from 0xfac00000!\n",*(unsigned *)0xfac00000);
    panic("FAIL :T_T \n");
}

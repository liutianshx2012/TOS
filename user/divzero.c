/*************************************************************************
	> File Name: divzero.c
	> Author: TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Sun 14 Aug 2016 01:17:51 AM PDT
 ************************************************************************/
#include <stdio.h>
#include <ulib.h>

int zero;

int
main(void) 
{
    cprintf("value is %d.\n",1/zero);
    panic("FAIL : T_T_C\n");
}

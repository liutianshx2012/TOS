/*************************************************************************
	> File Name: softint.c
	> Author: TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Sun 14 Aug 2016 09:05:49 PM PDT
 ************************************************************************/
#include <stdio.h>
#include <ulib.h>

int
main(void) 
{
     __asm__ volatile("int $14");
     panic("FAIL: T.T\n");
}



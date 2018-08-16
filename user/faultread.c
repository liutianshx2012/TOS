/*************************************************************************
	> File Name: faultread.c
	> Author: TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Sun 14 Aug 2016 01:52:38 AM PDT
 ************************************************************************/
#include <stdio.h>
#include <ulib.h>

int
main (void) 
{
    cprintf("I read %8x from 0.\n", *(unsigned int *) 0);
    panic("FAIL :T-T-C\n");
}

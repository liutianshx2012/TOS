/*************************************************************************
	> File Name: pgdir.c
	> Author: TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Fri 12 Aug 2016 07:36:17 AM PDT
 ************************************************************************/
#include <stdio.h>
#include <ulib.h>

int
main(void) 
{
    cprintf(" I am %d, printf pgdir.\n",getpid());
    print_pgdir();
    cprintf("pgdir pass.\n");
    return 0;
} 

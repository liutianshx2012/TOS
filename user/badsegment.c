/*************************************************************************
	> File Name: badsegment.c
	> Author: TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Fri 12 Aug 2016 05:58:20 PM PDT
 ************************************************************************/
#include <stdio.h>
#include <ulib.h>

/* try to load the kernel's TSS selector into the DS register */
int 
main(void) 
{
    __asm__ volatile(
        "movw $0x28,%ax; movw %ax,%ds"
    );

    panic("FAIL : TTc. \n ");
}


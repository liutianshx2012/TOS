/*************************************************************************
	> File Name: yield.c
	> Author: TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Sun 14 Aug 2016 09:12:21 PM PDT
 ************************************************************************/
#include <ulib.h>
#include <stdio.h>

int main(void)
{
    int i;
    cprintf("Hello, I am process %d.\n", getpid());
    for (i = 0; i < 5; i++) {
        yield();
        cprintf("Back in process %d, iteration %d.\n", getpid(), i);
    }
    cprintf("All done in process %d.\n", getpid());
    cprintf("yield pass.\n");
    return 0;
}

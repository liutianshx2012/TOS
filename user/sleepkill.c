/*************************************************************************
	> File Name: sleepkill.c
	> Author: TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Mon 15 Aug 2016 04:28:29 AM PDT
 ************************************************************************/
#include <stdio.h>
#include <ulib.h>

int 
main(void) 
{
    int pid;
    if ((pid = fork()) == 0) {
        sleep(~0);
        exit(0xdead);
    }
    assert(pid > 0);

    sleep(100);
    assert(kill(pid) == 0);
    cprintf("sleepkill pass .\n");
    return 0;
}

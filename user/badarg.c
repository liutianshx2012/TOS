/*************************************************************************
	> File Name: badarg.c
	> Author: TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Fri 12 Aug 2016 07:43:00 AM PDT
 ************************************************************************/
#include <stdio.h>
#include <ulib.h>

int
main(void) 
{
    int pid, exit_code;
    if ((pid = fork()) == 0) {
        cprintf("fork ok.\n");
        int i;
        for (i=0; i < 10;i++) {
            yield();
        }
        exit(0xbeaf);
    }
    assert(pid > 0);
    assert(waitpid(-1,NULL) != 0);
    assert(waitpid(pid,(void*)0xC0000000) !=0);
    assert(waitpid(pid,&exit_code) ==0 && exit_code == 0xbeaf);
    cprintf("badarg pass.\n");
    return 0;
}

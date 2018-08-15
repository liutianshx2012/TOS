/*************************************************************************
	> File Name: exit.c
	> Author: TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Mon 01 Aug 2016 11:16:33 PM PDT
 ************************************************************************/
#include <stdio.h>
#include <ulib.h>

int magic = -0x10384;

int 
main(void) 
{
    int pid = 0;
    int code = 0;
    cprintf("I am the parent .Forking the child ...\n");
    if ((pid == fork())==0 ) {
        cprintf(" I am the child .\n");
        yield();
        yield();
        yield();
        yield();
        yield();
        yield();
        yield();
        exit(magic);
    } else {
        cprintf("I am parent, fork a child pid %d\n",pid);
    }
    assert(pid > 0);
    cprintf("I am the parent, waiting now..\n");

    assert(waitpid(pid, &code) ==0 && code == magic);
    assert(waitpid(pid, &code) != 0 && wait() != 0);
    cprintf("waitpid %d ok.\n", pid);

    cprintf("exit pass.\n");
    return 0;
}
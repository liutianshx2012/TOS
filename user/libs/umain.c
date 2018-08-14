/*************************************************************************
	> File Name: umain.c
	> Author: TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Mon 01 Aug 2016 07:48:34 PM PDT
 ************************************************************************/
#include <ulib.h>

int main(void);

void
umain(void) 
{
    int ret = main();
    exit(ret);
}

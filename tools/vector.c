/*************************************************************************
	> File Name: vector.c
	> Author:TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 2016年06月18日 星期六 16时20分41秒
 ************************************************************************/
#include <stdio.h>
/* 生成vector.S,此文件包含了中断向量处理的统一实现 */
int
main(void)
{
    printf("# 包括256种各中断服务例程的入口地址和第一步初步处理实现;\n# Warning此文件是由tools/verctor.c在编译 kernel 期间动态生成的;\n");
    printf(".text\n");
    printf(".globl __alltraps\n");

    int i;
    for (i = 0; i < 256; i ++) {
        printf(".globl vector%d\n", i);
        printf("vector%d:\n", i);
        if (i != 8 && (i < 10 || i > 14) && i != 17) {
            printf("  pushl $0\n");
        }
        printf("  pushl $%d\n", i);
        printf("  jmp __alltraps\n");
    }
    printf("\n");
    printf("# vector table\n");
    printf(".data\n");
    printf(".globl __vectors\n");
    printf("__vectors:\n");
    for (i = 0; i < 256; i ++) {
        printf("  .long vector%d\n", i);
    }
    return 0;
}

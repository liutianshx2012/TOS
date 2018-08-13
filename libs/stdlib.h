/*************************************************************************
	> File Name: stdlib.c
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Sun 10 Jul 2016 06:26:45 PM CST
 ************************************************************************/
#ifndef __LIBS_STDLIB_H__
#define __LIBS_STDLIB_H__
#include <defs.h>

/* the largest number rand will return */
#define RAND_MAX    2147483647UL

/* libs/rand.c */
int rand(void);
void srand(unsigned int seed);

/* libs/hash.c */
uint32_t hash32(uint32_t val, unsigned int bits);

#endif /* !__LIBS_RAND_H__ */


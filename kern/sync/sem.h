/*************************************************************************
	> File Name: sem.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com 
	> Created Time: Mon 15 Aug 2016 08:54:44 PM PDT
 ************************************************************************/
#ifndef __KERN_SYNC_SEM_H
#define __KERN_SYNC_SEM_H

#include <defs.h>
#include <atomic.h>
#include <wait.h>

typedef struct
{
	int value;                  // 信号量的当前值
	wait_queue_t wait_queue;    // 信号量对应的等待队列
} semaphore_t;

void sem_init(semaphore_t *sem, int value);

void up(semaphore_t *sem);

void down(semaphore_t *sem);

bool try_down(semaphore_t *sem);

#endif

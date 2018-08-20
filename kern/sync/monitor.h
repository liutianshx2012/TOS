/*************************************************************************
	> File Name: monitor.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com 
	> Created Time: Mon 15 Aug 2016 10:13:27 PM PDT
 ************************************************************************/
#ifndef __KERN_SYNC_MONITOR_H
#define __KERN_SYNC_MONITOR_H

#include <sem.h>

typedef struct monitor monitor_t;

typedef struct condvar
{
    semaphore_t sem;  // the sem semaphore  is used to down the waiting proc, and the signaling proc should up the waiting proc
    int count;        // the number of waiters on condvar
    monitor_t *owner; // the owner(monitor) of this condvar
} condvar_t;

typedef struct monitor
{
    semaphore_t mutex; // the mutex lock for going into the routines in monitor, should be initialized to 1
    semaphore_t next;  // the next semaphore is used to down the signaling proc itself, and the other OR wakeuped waiting proc should wake up the sleeped signaling proc.
    int next_count;    // the number of of sleeped signaling proc
    condvar_t *cv;     // the condvars in monitor
} monitor_t;

// Initialize variables in monitor.
void monitor_init(monitor_t *cvp, size_t num_cv);
// Unlock one of threads waiting on the condition variable.
void cond_signal(condvar_t *cvp);
// Suspend calling thread on a condition variable waiting for condition atomically unlock mutex in monitor,
// and suspends calling thread on conditional variable after waking up locks mutex.
void cond_wait(condvar_t *cvp);

#endif

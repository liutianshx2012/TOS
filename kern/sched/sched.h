/*************************************************************************
	> File Name: sched.h
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Mon 18 Jul 2016 10:48:42 PM CST
 ************************************************************************/
#ifndef __KERN_SCHED_SCHED_H
#define __KERN_SCHED_SCHED_H

#include <defs.h>
#include <list.h>
#include <skew_heap.h>

#define MAX_TIME_SLICE 5

struct proc_struct;
struct run_queue;

typedef struct
{
	unsigned int expires;
	struct proc_struct *proc;
	list_entry_t timer_link;
} timer_t;

#define le2timer(le, member) \
	to_struct((le), timer_t, member)

static inline timer_t *
timer_init(timer_t *timer, struct proc_struct *proc, int expires)
{
	timer->expires = expires;
	timer->proc = proc;
	list_init(&(timer->timer_link));
	return timer;
}

// The introduction of scheduling classes is borrrowed from Linux, and makes the
// core scheduler quite extensible. These classes (the scheduler modules) encapsulate
// the scheduling policies.
struct sched_class
{
	// the name of sched_class
	const char *name;
	// Init the run queue
	void (*init)(struct run_queue *rq);
	// put the proc into runqueue, and this function must be called with rq_lock
	void (*enqueue)(struct run_queue *rq, struct proc_struct *proc);
	// get the proc out runqueue, and this function must be called with rq_lock
	void (*dequeue)(struct run_queue *rq, struct proc_struct *proc);
	// choose the next runnable task
	struct proc_struct *(*pick_next)(struct run_queue *rq);
	// dealer of the time-tick
	void (*proc_tick)(struct run_queue *rq, struct proc_struct *proc);
};

struct run_queue
{
	list_entry_t run_list;
	unsigned int proc_num;
	int max_time_slice;
	//for proj6 only
	skew_heap_entry_t *proj6_run_pool;
};

void sched_init(void);

void schedule(void);

void wakeup_proc(struct proc_struct *proc);

void add_timer(timer_t *timer);

void del_timer(timer_t *timer);

void run_timer_list(void);

#endif

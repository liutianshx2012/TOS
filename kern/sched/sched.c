/*************************************************************************
	> File Name: sched.c
	> Author:TTc 
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Mon 18 Jul 2016 10:48:48 PM CST
 ************************************************************************/
#include <list.h>
#include <sync.h>
#include <proc.h>
#include <assert.h>
#include <stdio.h>
#include <default_sched.h>
#include <sched.h>

// the list of timer
static list_entry_t timer_list;

static struct sched_class *sched_class;

static struct run_queue *rq;

static inline void
sched_class_enqueue(struct proc_struct *proc) 
{
    if (proc != idleproc) {
        sched_class->enqueue(rq, proc);
    }
}

static inline void
sched_class_dequeue(struct proc_struct *proc) 
{
    sched_class->dequeue(rq, proc);
}

static inline struct proc_struct *
sched_class_pick_next(void) 
{
    return sched_class->pick_next(rq);
}

static void
sched_class_proc_tick(struct proc_struct *proc) 
{
    if (proc != idleproc) {
        sched_class->proc_tick(rq, proc);
    } else {
        proc->need_resched = 1;
    }
}

static struct run_queue __rq;

void
sched_init(void) 
{
    list_init(&timer_list);

    sched_class = &default_sched_class;

    rq = &__rq;
    rq->max_time_slice = MAX_TIME_SLICE;
    sched_class->init(rq);

    cprintf("sched class: %s\n", sched_class->name);
}

void
wakeup_proc(struct proc_struct *proc) 
{
    assert(proc->state != PROC_ZOMBIE); 
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        if (proc->state != PROC_RUNNABLE) {
            proc->state = PROC_RUNNABLE;
            proc->wait_state = 0;
            if (proc != current) {
                sched_class_enqueue(proc);
            }
        } else {
            warn("wakeup runnable process.\n");
        }
    }
    local_intr_restore(intr_flag);
}

void
schedule(void) 
{
    bool intr_flag;
    struct proc_struct *next;
    local_intr_save(intr_flag);
    {
        current->need_resched = 0;
        if (current->state == PROC_RUNNABLE) {
            sched_class_enqueue(current);
        }
        if ((next = sched_class_pick_next()) != NULL) {
            sched_class_dequeue(next);
        }
        if (next == NULL) {
            next = idleproc;
        }

        next->runs ++;
        if (next != current) {
            proc_run(next);
        }
    }
    local_intr_restore(intr_flag);
}
// 向系统添加某个初始化过的 timer ,该定时器在指定时间后被激活,并将对应的进程唤醒至 runnable(如果当前处于等待状态).
void
add_timer(timer_t *timer)
{
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        assert(timer->expires > 0 && timer->proc != NULL);
        assert(list_empty(&(timer->timer_link)));
        list_entry_t *le = list_next(&timer_list);
        while (le != &timer_list) {
            timer_t *next = le2timer(le, timer_link);
            if (timer->expires < next->expires) {
                next->expires -= timer->expires;
                break;
            }
            timer->expires -= next->expires;
            le = list_next(le);
        }
        list_add_before(le, &(timer->timer_link));
    }
    local_intr_restore(intr_flag);
}
// 取消某一个定时器.
void
del_timer(timer_t *timer)
{
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        if (!list_empty(&(timer->timer_link))) {
            if (timer->expires != 0) {
                list_entry_t *le = list_next(&(timer->timer_link));
                if (le != &timer_list) {
                    timer_t *next = le2timer(le, timer_link);
                    next->expires += timer->expires;
                }
            }
            list_del_init(&(timer->timer_link));
        }
    }
    local_intr_restore(intr_flag);
}
// 更新当前系统时间点,遍历当前所有处在系统管理内的定时器,找出所有应该激活的 timer ,并激活它们.
// 该过程在且只在每次 时钟中断 时被调用.
void
run_timer_list(void)
{
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        list_entry_t *le = list_next(&timer_list);
        if (le != &timer_list) {
            timer_t *timer = le2timer(le, timer_link);
            assert(timer->expires != 0);
            timer->expires --;
            while (timer->expires == 0) {
                le = list_next(le);
                struct proc_struct *proc = timer->proc;
                if (proc->wait_state != 0) {
                    assert(proc->wait_state & WT_INTERRUPTED);
                } else {
                    warn("process %d's wait_state == 0.\n", proc->pid);
                }
                wakeup_proc(proc); 
                del_timer(timer);
                if (le == &timer_list) {
                    break;
                }
                timer = le2timer(le, timer_link);
            }
        }
        sched_class_proc_tick(current);// 调度
    }
    local_intr_restore(intr_flag);
}
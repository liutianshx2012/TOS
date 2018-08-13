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
#include <sched.h>

void
wakeup_proc(struct proc_struct *proc) 
{
    assert(proc->state != PROC_ZOMBIE && proc->state != PROC_RUNNABLE);
    proc->state = PROC_RUNNABLE;
}

void
schedule(void) 
{
    cprintf("cpu schedule!!\n");
    bool intr_flag;
    list_entry_t *le;
    list_entry_t *last;
    struct proc_struct *next = NULL;
    local_intr_save(intr_flag);
    {
        current->need_resched = 0;
        last = (current == idleproc) ? &proc_list : &(current->list_link);
        le = last;

        do {
            if ((le = list_next(le)) != &proc_list) {
                next = le2proc(le, list_link);
                if (next->state == PROC_RUNNABLE) {
                    cprintf("find a PROC_RUNNABLE =>[%s]\n",next->name);
                    break;
                }
            }
        } while (le != last);

        if (next == NULL || next->state != PROC_RUNNABLE) {
            next = idleproc;
        }

        next->runs ++;

        if (next != current) {
            proc_run(next);
        }
    }
    local_intr_restore(intr_flag);
}



/*************************************************************************
> File Name: default_sched.c
> Author: TTc
> Mail: liutianshxkernel@gmail.com
> Created Time: Sun 14 Aug 2016 06:36:49 PM PDT
************************************************************************/
#include <defs.h>
#include <list.h>
#include <proc.h>
#include <assert.h>
#include <default_sched.h>

#define USE_SKEW_HEAP   1

/* you should define the BigStride constant here*/
/* proj6 : your code*/
#define BIG_STRIDE   0x7FFFFFFF

/* The compare function for two skew_heap_node_t's and the corresponding procs*/
static int
proc_stride_comp_f(void *a ,void *b) {
    struct proc_struct *p = le2proc(a,proj6_run_pool);
    struct proc_struct *q = le2proc(b,proj6_run_pool);
    int32_t c = p->proj6_stride - q->proj6_stride;
    if (c > 0) return 1;
    else if (c == 0) return 0;
    else return -1;
}

/* stride_init initializes the run-queue rq with correct assignment member variables, including:
*
* runlist : should be a empty list after initializes.
* proj6_run_pool: NULL.
* proc_num: 0;
* max_time_slice : no need here, the variables would be assigned by the caller.
* hint:  routines of the list structures.
* */
static void
stride_init(struct run_queue *rq) {
    /*proj6 : your code */
    list_init(&(rq->run_list));
    rq->proj6_run_pool = NULL;
    rq->proc_num = 0;
}

/* stride_enqueue inserts the process 'proc' into the run-queue
* 'rq'. The procedure should verift/initialize the relevant members of 'proc', and then put the 'proj6_run_pool' node into the queue(since we use priority queue here). The procedure should also update the meta date in 'rq' structure.
*
*proc->time_slice denotes the time slices allocation for the process, which should set to rq->max_time_slice.
*
* hint: for routines of the priority queue structures.
*
* */
static void
stride_enqueue(struct run_queue *rq , struct proc_struct *proc) {
    /* proj6: YOUR CODE */
#if USE_SKEW_HEAP
    rq->proj6_run_pool =
        skew_heap_insert(rq->proj6_run_pool, &(proc->proj6_run_pool), proc_stride_comp_f);
#else
    assert(list_empty(&(proc->run_link)));
    list_add_before(&(rq->run_list), &(proc->run_link));
#endif
    if (proc->time_slice == 0 || proc->time_slice > rq->max_time_slice){
        proc->time_slice = rq->max_time_slice;
    }
    proc->rq = rq;
    rq->proc_num ++;
}
/*
** stride_dequeue removes the process ``proc'' from the run-queue
** ``rq'', the operation would be finished by the skew_heap_remove
** operations. Remember to update the ``rq'' structure.
*
** hint: for routines of the priority
** queue structures.
*/
static void
stride_dequeue(struct run_queue *rq, struct proc_struct *proc) {
    /* proj6: YOUR CODE */
#if USE_SKEW_HEAP
    rq->proj6_run_pool =
        skew_heap_remove(rq->proj6_run_pool, &(proc->proj6_run_pool), proc_stride_comp_f);
#else
    assert(!list_empty(&(proc->run_link)) && proc->rq == rq);
    list_del_init(&(proc->run_link));
#endif
    rq->proc_num --;
}
/*
** stride_pick_next pick the element from the ``run-queue'', with the
** minimum value of stride, and returns the corresponding process
** pointer. The process pointer would be calculated by macro le2proc,
** see proj13.1/kern/process/proc.h for definition. Return NULL if
** there is no process in the queue.
**
** When one proc structure is selected, remember to update the stride
** property of the proc. (stride += BIG_STRIDE / priority)
**
** for routines of the priority
** queue structures.
**/
static struct proc_struct *
stride_pick_next(struct run_queue *rq) {
    /* proj6: YOUR CODE */
#if USE_SKEW_HEAP
    if (rq->proj6_run_pool == NULL) return NULL;
    struct proc_struct *p = le2proc(rq->proj6_run_pool, proj6_run_pool);
#else
    list_entry_t *le = list_next(&(rq->run_list));

    if (le == &rq->run_list)
        return NULL;

    struct proc_struct *p = le2proc(le, run_link);
    le = list_next(le);
    while (le != &rq->run_list) {
        struct proc_struct *q = le2proc(le, run_link);
        if ((int32_t)(p->proj6_stride - q->proj6_stride) > 0)
            p = q;
        le = list_next(le);
    }
#endif
    if (p->proj6_priority == 0)
        p->proj6_stride += BIG_STRIDE;
    else p->proj6_stride += BIG_STRIDE / p->proj6_priority;
    return p;
}

/*
*  * stride_proc_tick works with the tick event of current process. You
*   * should check whether the time slices for current process is
*    * exhausted and update the proc struct ``proc''. proc->time_slice
*     * denotes the time slices left for current
*      * process. proc->need_resched is the flag variable for process
*       * switching.
*        */
static void
stride_proc_tick(struct run_queue *rq, struct proc_struct *proc) {
    /* proj6: YOUR CODE */
     if (proc->time_slice > 0) {
          proc->time_slice --;
     }
     if (proc->time_slice == 0) {
          proc->need_resched = 1;
     }
}

struct sched_class default_sched_class = {
     .name = "stride_scheduler",
     .init = stride_init,
     .enqueue = stride_enqueue,
     .dequeue = stride_dequeue,
     .pick_next = stride_pick_next,
     .proc_tick = stride_proc_tick,
};

/*************************************************************************
	> File Name: sem.c
	> Author:TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: Mon 15 Aug 2016 09:47:00 PM PDT
 ************************************************************************/
#include <defs.h>
#include <wait.h>
#include <atomic.h>
#include <kmalloc.h>
#include <sem.h>
#include <proc.h>
#include <sync.h>
#include <assert.h>


void
sem_init(semaphore_t *sem, int value) 
{
    sem->value = value;
    wait_queue_init(&(sem->wait_queue));
}
// 具体实现信号量的 V 操作, 首先关中断
// 如果信号量对应的 wait_queue 中没有进程在等待,直接把信号量的 value +1, 然后开中断返回;   
// 如果有进程在等待且进程等待的原因是 semophore 设置的,则调用 wakeup_wait 函数将 waitqueue 中等待的第一个 wait 删除,且把此 wait 关联的进程唤醒,最后开中断返回. 
static __noinline void
__up(semaphore_t *sem, uint32_t wait_state) 
{
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        wait_t *wait;
        if ((wait = wait_queue_first(&(sem->wait_queue))) == NULL) {
            sem->value ++;
        } else{
            assert(wait->proc->wait_state == wait_state);
            wakeup_wait(&(sem->wait_queue), wait, wait_state, 1);
        }
    }
    local_intr_restore(intr_flag);
}
// 具体实现 P 操作, 首先关掉中断, 然后判断当前信号量的 value 是否 > 0. 
// 如果 value > 0 ,则表明可以获得信号量, 故让 value -1,并打开中断返回即可.
// 如果 value !> 0 ,则表明无法获得信号量,故需要将当前的进程加入等待队列 wait_queue,并打开中断,然后运行调度器选择另外一个就绪的进程执行.
// 如果被 V 操作唤醒,则把自身关联的 wait 从等待队列中删除(此过程需要先关中断,完成后开中断).
static __noinline uint32_t
__down(semaphore_t *sem, uint32_t wait_state) 
{
    bool intr_flag;
    local_intr_save(intr_flag);
    if (sem->value > 0) {
        sem->value --;
        local_intr_restore(intr_flag);
        return 0;
    }
    wait_t __wait, *wait = &__wait;
    wait_current_set(&(sem->wait_queue), wait, wait_state);
    local_intr_restore(intr_flag);

    schedule();

    local_intr_save(intr_flag);
    wait_current_del(&(sem->wait_queue), wait);
    local_intr_restore(intr_flag);

    if (wait->wakeup_flags != wait_state) {
        return wait->wakeup_flags;
    }
    return 0;
}
// V 操作
void
up(semaphore_t *sem) 
{
    __up(sem, WT_KSEM);
}
// P 操作
void
down(semaphore_t *sem) 
{
    uint32_t flags = __down(sem, WT_KSEM);
    assert(flags == 0);
}

bool
try_down(semaphore_t *sem) 
{
    bool intr_flag, ret = 0;
    local_intr_save(intr_flag);
    if (sem->value > 0) {
        sem->value --, ret = 1;
    }
    local_intr_restore(intr_flag);
    return ret;
}

#include "scheduler.h"

static pcb_t *ready_queue = 0;
static pcb_t *last_process = 0;

void scheduler_init(void)
{
    ready_queue = 0;
    last_process = 0;
}

void scheduler_add(pcb_t *process)
{
    if (process == 0) {
        return;
    }

    process->state = PROC_READY;
    process->next = 0;

    if (ready_queue == 0) {
        ready_queue = process;
        last_process = process;
        return;
    }

    last_process->next = process;
    last_process = process;
}

pcb_t *scheduler_next(void)
{
    pcb_t *current;

    if (ready_queue == 0) {
        return 0;
    }

    current = ready_queue;

    ready_queue = ready_queue->next;

    if (ready_queue == 0) {
        last_process = 0;
    }

    current->next = 0;

    return current;
}

void scheduler_tick(void)
{
    pcb_t *next;

    next = scheduler_next();

    if (next != 0) {
        next->state = PROC_RUNNING;

        scheduler_add(next);
    }
}
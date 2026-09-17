#include "process.h"

static pcb_t processes[MAX_PROCESSES];

static pcb_t *current_process = 0;

static uint32_t process_count = 0;

void process_init(void)
{
    process_count = 0;
    current_process = 0;

    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        processes[i].pid = 0;
        processes[i].state = PROC_TERMINATED;
        processes[i].esp = 0;
        processes[i].eip = 0;
        processes[i].next = 0;
    }
}

int process_create(void (*entry)(void))
{
    if (process_count >= MAX_PROCESSES) {
        return -1;
    }

    pcb_t *p = &processes[process_count];

    p->pid = process_count + 1;
    p->state = PROC_READY;

    p->eip = (uint32_t)entry;

    p->esp = (uint32_t)&p->stack[(STACK_SIZE / 4) - 1];

    p->next = 0;

    if (current_process == 0) {
        current_process = p;
        p->state = PROC_RUNNING;
    }

    process_count++;

    return (int)p->pid;
}

void process_yield(void)
{
    if (current_process != 0) {
        current_process->state = PROC_READY;
    }
}

void process_exit(void)
{
    if (current_process != 0) {
        current_process->state = PROC_TERMINATED;
    }
}

pcb_t *process_get_current(void)
{
    return current_process;
}

pcb_t *process_get_list(void)
{
    if (process_count == 0) {
        return 0;
    }

    return &processes[0];
}
#ifndef PROCESS_H
#define PROCESS_H

#include "../include/types.h"

#define MAX_PROCESSES 8
#define STACK_SIZE 4096

typedef enum {
    PROC_READY,
    PROC_RUNNING,
    PROC_TERMINATED
} proc_state_t;

typedef struct pcb {
    uint32_t pid;
    proc_state_t state;

    uint32_t esp;
    uint32_t eip;

    uint32_t stack[STACK_SIZE / 4];

    struct pcb *next;
} pcb_t;

void process_init(void);

int process_create(void (*entry)(void));

void process_yield(void);

void process_exit(void);

pcb_t *process_get_current(void);

pcb_t *process_get_list(void);

#endif
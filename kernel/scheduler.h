#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

void scheduler_init(void);

void scheduler_add(pcb_t *process);

pcb_t *scheduler_next(void);

void scheduler_tick(void);

#endif
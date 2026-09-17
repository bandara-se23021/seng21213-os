#ifndef SEMAPHORE_H
#define SEMAPHORE_H

typedef struct
{
    volatile int value;
} semaphore_t;

void sem_init(semaphore_t *sem, int value);
void sem_wait(semaphore_t *sem);
void sem_signal(semaphore_t *sem);

#endif
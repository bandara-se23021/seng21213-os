#ifndef THREAD_H
#define THREAD_H

#include "types.h"

#define MAX_THREADS 16
#define THREAD_STACK_SIZE 4096

typedef enum {
    THREAD_UNUSED,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_FINISHED
} thread_state_t;

typedef void (*thread_function_t)(void *arg);

typedef struct {
    int id;
    thread_state_t state;

    thread_function_t function;
    void *arg;

    uint8_t stack[THREAD_STACK_SIZE];
} thread_t;

void thread_init(void);

int thread_create(thread_function_t function, void *arg);

void thread_run(int thread_id);

void thread_yield(void);

void thread_exit(void);

thread_t *thread_get(int thread_id);

#endif
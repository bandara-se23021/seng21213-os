#include "thread.h"
#include "vga.h"

static thread_t threads[MAX_THREADS];

static int current_thread = -1;
static int next_thread_id = 0;

void thread_init(void)
{
    for (int i = 0; i < MAX_THREADS; i++)
    {
        threads[i].id = -1;
        threads[i].state = THREAD_UNUSED;
        threads[i].function = 0;
        threads[i].arg = 0;
    }

    current_thread = -1;
    next_thread_id = 0;
}

int thread_create(thread_function_t function, void *arg)
{
    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (threads[i].state == THREAD_UNUSED)
        {
            threads[i].id = next_thread_id++;
            threads[i].state = THREAD_READY;
            threads[i].function = function;
            threads[i].arg = arg;

            return threads[i].id;
        }
    }

    return -1;
}

thread_t *thread_get(int thread_id)
{
    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (threads[i].id == thread_id &&
            threads[i].state != THREAD_UNUSED)
        {
            return &threads[i];
        }
    }

    return 0;
}

void thread_run(int thread_id)
{
    thread_t *thread = thread_get(thread_id);

    if (thread == 0)
    {
        return;
    }

    current_thread = thread_id;
    thread->state = THREAD_RUNNING;

    if (thread->function != 0)
    {
        thread->function(thread->arg);
    }

    thread->state = THREAD_FINISHED;
    current_thread = -1;
}

void thread_yield(void)
{
    /*
     * Basic Stage 2 implementation.
     *
     * Full context switching can be added
     * when the scheduler/interrupt mechanism
     * is implemented.
     */
}

void thread_exit(void)
{
    if (current_thread >= 0)
    {
        thread_t *thread = thread_get(current_thread);

        if (thread != 0)
        {
            thread->state = THREAD_FINISHED;
        }
    }
}
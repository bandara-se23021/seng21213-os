#include "semaphore.h"

void sem_init(semaphore_t *sem, int value)
{
    sem->value = value;
}

void sem_wait(semaphore_t *sem)
{
    while (1)
    {
        int old_value;

        do
        {
            old_value = sem->value;

            if (old_value <= 0)
            {
                break;
            }

        } while (!__sync_bool_compare_and_swap(
            &sem->value,
            old_value,
            old_value - 1
        ));

        if (old_value > 0)
        {
            return;
        }
    }
}

void sem_signal(semaphore_t *sem)
{
    __sync_fetch_and_add(&sem->value, 1);
}
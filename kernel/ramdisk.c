#include "ramdisk.h"

/*
 * RAM disk is located at physical address 1 MB.
 */
static uint8_t *ramdisk =
    (uint8_t *)RAMDISK_ADDRESS;

/*
 * Initialise the 1 MB RAM disk.
 *
 * Initially every byte is cleared.
 */
void ramdisk_init(void)
{
    uint32_t i;

    for (i = 0; i < RAMDISK_SIZE; i++)
    {
        ramdisk[i] = 0;
    }
}

/*
 * Return pointer to RAM disk memory.
 */
uint8_t *ramdisk_data(void)
{
    return ramdisk;
}
#ifndef RAMDISK_H
#define RAMDISK_H

#include "../include/types.h"

#define RAMDISK_SIZE       (1024 * 1024)
#define RAMDISK_BLOCK_SIZE 4096
#define RAMDISK_BLOCKS     (RAMDISK_SIZE / RAMDISK_BLOCK_SIZE)

/*
 * RAM disk is placed at physical address 1 MB.
 *
 * Kernel is loaded at 0x10000 and stack is at 0x90000,
 * so 0x100000 is used for the Stage 4 RAM disk.
 */
#define RAMDISK_ADDRESS 0x00100000

void ramdisk_init(void);

uint8_t *ramdisk_data(void);

#endif
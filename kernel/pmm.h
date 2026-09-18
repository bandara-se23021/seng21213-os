#ifndef PMM_H
#define PMM_H

#include "../include/types.h"

#define PAGE_SIZE 4096

#define E820_COUNT_ADDRESS 0x4FFC
#define E820_MAP_ADDRESS   0x5000

#define E820_USABLE 1

typedef struct
{
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} e820_entry_t;

void pmm_init(void);

uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t paddr);

uint32_t pmm_get_total_memory(void);
uint32_t pmm_get_used_memory(void);
uint32_t pmm_get_free_memory(void);

void pmm_test(void);

#endif
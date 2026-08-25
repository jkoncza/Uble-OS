#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>

#define MEMORY_PAGE_SIZE 4096ULL

bool memory_init(void);

void *memory_alloc_page(uint64_t *physical_address);

void *memory_alloc_pages(
    uint64_t page_count,
    uint64_t *physical_address
);

void *memory_phys_to_virt(
    uint64_t physical_address
);

uint64_t memory_virt_to_phys(
    const void *virtual_address
);

uint64_t memory_get_total_usable(void);

uint64_t memory_get_allocated(void);

bool memory_is_initialized(void);

#endif
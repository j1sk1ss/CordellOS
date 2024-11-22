#ifndef PMM_H_
#define PMM_H_

#include <memory.h>
#include <stdint.h>

#include "stdio.h"


#define MEM_OFFSET      0x10000
#define BLOCK_SIZE      4096     // Size of 1 block of memory, 4KB
#define BLOCKS_PER_BYTE 8        // Using a bitmap, each byte will hold 8 bits/blocks


typedef struct pmm_map {
    uint32_t* memory_map;
    uint32_t max_blocks;
    uint32_t used_blocks;
} pmma_map_t;


// Global variables
extern pmma_map_t PMM_map;


void PMM_init(const uint32_t start_address, size_t size);
void PMM_deinitialize_memory_region(const uint32_t base_address, size_t size);
void PMM_initialize_memory_region(const uint32_t base_address, size_t size);
uint32_t* PMM_allocate_blocks(size_t num_blocks);
void PMM_free_blocks(const uint32_t* address, size_t num_blocks);

void _set_block(const uint32_t bit);
void _unset_block(const uint32_t bit);
int32_t _find_first_free_blocks(size_t num_blocks);

#endif
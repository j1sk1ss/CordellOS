/* https://github.com/queso-fuego/amateuros/blob/master/include/memory/physical_memory_manager.h#L80 */

#include "../include/pmm.h"


pmma_map_t PMM_map = {
    .max_blocks = 0,
    .memory_map = NULL,
    .used_blocks = 0
};


void PMM_init(const uint32_t start_address, const uint32_t size) {
    PMM_map.memory_map = (uint32_t*)start_address;
    PMM_map.max_blocks = size / BLOCK_SIZE;
    PMM_map.used_blocks = PMM_map.max_blocks;
    memset32(PMM_map.memory_map, 0xFFFFFFFF, PMM_map.max_blocks / BLOCKS_PER_BYTE);
}

void PMM_initialize_memory_region(const uint32_t base_address, const uint32_t size) {
    int32_t align = base_address / BLOCK_SIZE;
    int32_t num_blocks = size / BLOCK_SIZE;

    for (; num_blocks > 0; num_blocks--) {
        _unset_block(align++);
        PMM_map.used_blocks--;
    }
}

void PMM_deinitialize_memory_region(const uint32_t base_address, const uint32_t size) {
    int32_t align      = base_address / BLOCK_SIZE;
    int32_t num_blocks = size / BLOCK_SIZE;

    for (; num_blocks > 0; num_blocks--) {
        _set_block(align++);
        PMM_map.used_blocks++;
    }
}

void _set_block(const uint32_t bit) {
    PMM_map.memory_map[bit / 32] |= (1 << (bit % 32));
}

void _unset_block(const uint32_t bit) {
    PMM_map.memory_map[bit / 32] &= ~(1 << (bit % 32));
}

int32_t _find_first_free_blocks(const uint32_t num_blocks) {
    if (num_blocks == 0) return -1;
    for (uint32_t i = 0; i < PMM_map.max_blocks / 32;  i++) 
        if (PMM_map.memory_map[i] != 0xFFFFFFFF) 
            for (int32_t j = 0; j < 32; j++) {
                int32_t bit = 1 << j;
                if (!(PMM_map.memory_map[i] & bit)) 
                    for (uint32_t count = 0, _free_blocks = 0; count < num_blocks; count++) {
                        if ((j + count > 31) && (i + 1 < PMM_map.max_blocks / 32)) {
                            if (!(PMM_map.memory_map[i + 1] & (1 << ((j + count) - 32)))) _free_blocks++;
                        }
                        else {
                            if (!(PMM_map.memory_map[i] & (1 << (j + count)))) _free_blocks++;
                        }

                        if (_free_blocks == num_blocks) return i * 32 + j;
                    }
            }
    
    return -1;
}

uint32_t* _allocate_blocks(const uint32_t num_blocks) {
    if ((PMM_map.max_blocks - PMM_map.used_blocks) <= num_blocks) return 0;   
    int32_t starting_block = _find_first_free_blocks(num_blocks);
    if (starting_block == -1) return 0;

    for (uint32_t i = 0; i < num_blocks; i++)
        _set_block(starting_block + i);

    PMM_map.used_blocks += num_blocks;
    uint32_t address = starting_block * BLOCK_SIZE + (uint32_t)PMM_map.memory_map;
    return (uint32_t*)address;
}

void _free_blocks(const uint32_t *address, const uint32_t num_blocks) {
    int32_t starting_block = (uint32_t)address / BLOCK_SIZE;
    for (uint32_t i = 0; i < num_blocks; i++) _unset_block(starting_block + i);
    PMM_map.used_blocks -= num_blocks;
}
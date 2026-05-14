/* https://github.com/queso-fuego/amateuros/blob/master/include/memory/physical_memory_manager.h#L80 */

#include "../include/pmm.h"

pmma_map_t PMM_map = {
    .max_blocks  = 0,
    .memory_map  = NULL,
    .used_blocks = 0
};

static int _set_block(const uint32_t bit) {
    PMM_map.memory_map[bit / 32] |= (1 << (bit % 32));
    return 1;
}

static int _unset_block(const uint32_t bit) {
    PMM_map.memory_map[bit / 32] &= ~(1 << (bit % 32));
    return 1;
}

static int _test_block(const uint32_t bit) {
    return (PMM_map.memory_map[bit / 32] & (1 << (bit % 32))) != 0;
}

static int _find_first_free_blocks(size_t num_blocks) {
    if (num_blocks == 0) return -1;

    for (uint32_t bit = 1; bit + num_blocks <= PMM_map.max_blocks; bit++) {
        uint32_t free_blocks = 0;
        while (free_blocks < num_blocks && !_test_block(bit + free_blocks)) {
            free_blocks++;
        }

        if (free_blocks == num_blocks) return bit;
        bit += free_blocks;
    }
    
    return -1;
}

int PMM_init(const uint32_t start_address, size_t size) {
    PMM_map.memory_map  = (uint32_t*)start_address;
    PMM_map.max_blocks  = size / BLOCK_SIZE;
    PMM_map.used_blocks = PMM_map.max_blocks;

    uint32_t map_size = (PMM_map.max_blocks + BLOCKS_PER_BYTE - 1) / BLOCKS_PER_BYTE;
    memset(PMM_map.memory_map, 0xFFFFFFFF, map_size);
    return 1;
}

int PMM_initialize_memory_region(const uint32_t base_address, size_t size) {
    if (size == 0) return 1;

    uint32_t start_block = base_address / BLOCK_SIZE;
    uint64_t end_address = (uint64_t)base_address + size;
    uint64_t end_block64 = (end_address + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint32_t end_block = end_block64 > PMM_map.max_blocks ? PMM_map.max_blocks : (uint32_t)end_block64;

    for (uint32_t block = start_block; block < end_block; block++) {
        if (_test_block(block)) {
            _unset_block(block);
            PMM_map.used_blocks--;
        }
    }

    return 1;
}

int PMM_deinitialize_memory_region(const uint32_t base_address, size_t size) {
    if (size == 0) return 1;

    uint32_t start_block = base_address / BLOCK_SIZE;
    uint64_t end_address = (uint64_t)base_address + size;
    uint64_t end_block64 = (end_address + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint32_t end_block = end_block64 > PMM_map.max_blocks ? PMM_map.max_blocks : (uint32_t)end_block64;

    for (uint32_t block = start_block; block < end_block; block++) {
        if (!_test_block(block)) {
            _set_block(block);
            PMM_map.used_blocks++;
        }
    }

    return 1;
}

uint32_t* PMM_allocate_blocks(size_t num_blocks) {
    if (num_blocks == 0) return NULL;
    if ((PMM_map.max_blocks - PMM_map.used_blocks) < num_blocks) return NULL;
    int32_t starting_block = _find_first_free_blocks(num_blocks);
    if (starting_block == -1) return NULL;

    for (uint32_t i = 0; i < num_blocks; i++)
        _set_block(starting_block + i);

    PMM_map.used_blocks += num_blocks;
    uint32_t address = starting_block * BLOCK_SIZE;
    return (uint32_t*)address;
}

int PMM_free_blocks(const uint32_t *address, size_t num_blocks) {
    int32_t starting_block = (uint32_t)address / BLOCK_SIZE;
    for (uint32_t i = 0; i < num_blocks; i++) _unset_block(starting_block + i);
    PMM_map.used_blocks -= num_blocks;
    return 1;
}

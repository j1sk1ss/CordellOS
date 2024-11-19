#ifndef ALLOCATOR_H_
#define ALLOCATOR_H_


#include <stdint.h>
#include <stddef.h>
#include <memory.h>
#include <assert.h>

#include "phys_manager.h"
#include "virt_manager.h"


#define PAGE_SIZE               4096
#define MAX_PAGE_ALIGNED_ALLOCS 32


typedef struct malloc_block {
    uint32_t size;
    bool free;

    uint32_t v_addr;
    uint32_t pcount;

    struct malloc_block *next;
} malloc_block_t;


extern malloc_block_t *kmalloc_list_head;
extern malloc_block_t *umalloc_list_head;

extern uint32_t malloc_virt_address;
extern uint32_t kmalloc_phys_addresss;
extern uint32_t total_malloc_pages;


uint32_t kmalloc_total_free();
uint32_t kmalloc_total_avaliable();
uint32_t umalloc_total_avaliable();
void print_malloc_map();

void kmm_init(const uint32_t bytes);
void umm_init(const uint32_t bytes);

void block_split(malloc_block_t *node, const uint32_t size);
void* kmalloc(malloc_block_t* entry, const uint32_t size, uint8_t type);
void* krealloc(void* ptr, size_t size);
void* kmallocp(uint32_t v_addr, uint8_t type);

void cleanup(malloc_block_t* entry, void* ptr);
void merge_free_blocks(malloc_block_t* block);
void kfree(malloc_block_t* entry, void* ptr);
void kfreep(void* v_addr);

#endif
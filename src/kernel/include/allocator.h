#pragma once

#include <stdint.h>
#include <stddef.h>

#include "phys_manager.h"
#include "virt_manager.h"

#include "../../libs/include/memory.h"


#define PAGE_SIZE               4096
#define MAX_PAGE_ALIGNED_ALLOCS 32


typedef struct malloc_block {
    uint32_t size;
    bool free;
    struct malloc_block *next;
} malloc_block_t;


extern malloc_block_t *malloc_list_head;
extern uint32_t malloc_virt_address;
extern uint32_t malloc_phys_address;
extern uint32_t total_malloc_pages;

uint32_t kmalloc_total_free();
uint32_t kmalloc_total_avaliable();
void print_kmalloc_map();

void mm_init(const uint32_t bytes);
void kmalloc_split(malloc_block_t *node, const uint32_t size);
void *kmalloc(const uint32_t size);
void merge_free_blocks();
void kfree(void *ptr);
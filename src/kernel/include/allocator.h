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

typedef struct malloc_head {
	malloc_block_t* list_head;
	uint32_t virt_address;
	uint32_t phys_address;
	uint32_t total_pages;
    bool (*map_page)(void* virt, void* phys);
} malloc_head_t;


void* kmalloc(const uint32_t size);
void* krealloc(void* ptr, size_t size);
void* umalloc(const uint32_t size);
void* urealloc(void* ptr, size_t size);
int kmallocp(uint32_t v_addr);
int umallocp(uint32_t v_addr);
void kfreep(void* v_addr);
int kfree(void* ptr);
int ufree(void* ptr);

int _mm_init(const uint32_t bytes, malloc_head_t* head);
void* _kmalloc(size_t size, malloc_head_t* head);
int _kmallocp(uint32_t virt, malloc_head_t* head);
void* _krealloc(void* ptr, size_t size, malloc_head_t* head);
int _kfree(void* ptr, malloc_head_t* head);
int _block_split(malloc_block_t *node, const uint32_t size);
int _merge_free_blocks(malloc_block_t* block);

uint32_t kmalloc_total_free();
uint32_t kmalloc_total_avaliable();
uint32_t umalloc_total_avaliable();
void print_malloc_map();

#endif
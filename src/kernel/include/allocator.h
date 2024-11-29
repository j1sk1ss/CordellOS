#ifndef ALLOCATOR_H_
#define ALLOCATOR_H_

#include <stdint.h>
#include <stdint.h>
#include <stddef.h>
#include <memory.h>
#include <assert.h>

#include "pmm.h"
#include "vmm.h"


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
    int (*map_page)(void*, void*);
    void* (*mkpage)(uint32_t*, uint8_t);
} malloc_head_t;


void* ALC_malloc(size_t size, uint8_t type);
void* _kmalloc(size_t size);
void* _umalloc(size_t size);

void* ALC_realloc(void* ptr, size_t size, uint8_t type);
void* _krealloc(void* ptr, size_t size);
void* _urealloc(void* ptr, size_t size);

int ALC_mallocp(uint32_t v_addr, uint8_t type);
int _kmallocp(uint32_t v_addr);
int _umallocp(uint32_t v_addr);

int ALC_free(void* ptr, uint8_t type);
int _kfree(void* ptr);
int _ufree(void* ptr);

void _kfreep(void* v_addr);

int __mm_init(size_t bytes, malloc_head_t* head);
void* __kmalloc(size_t size, malloc_head_t* head, uint8_t type);
int __kmallocp(uint32_t virt, malloc_head_t* head, uint8_t type);
void* __krealloc(void* ptr, size_t size, malloc_head_t* head, uint8_t type);
int __kfree(void* ptr, malloc_head_t* head);
int __block_split(malloc_block_t *node, size_t size);
int __merge_free_blocks(malloc_block_t* block);

int kprint_kmalloc();
int kprint_umalloc();
int _print_malloc(malloc_head_t* head);

#endif
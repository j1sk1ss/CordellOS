#include <allocator.h>

static void* __kmalloc(size_t, malloc_head_t*, uint8_t);
static int   __kmallocp(uint32_t, malloc_head_t*);
static void* __krealloc(void*, size_t, malloc_head_t*, uint8_t);
static int   __kfree(void*, malloc_head_t*);
static int   __block_split(malloc_block_t*, size_t);
static int   __merge_free_blocks(malloc_block_t*);

malloc_head_t _kernel_malloc = {
	.list_head    = NULL,
	.phys_address = 0,
	.total_pages  = 0,
	.virt_address = KERNEL_HEAP_START,
	.map_page     = VMM_kmap_page
};

malloc_head_t _user_malloc = {
	.list_head    = NULL,
	.phys_address = 0,
	.total_pages  = 0,
	.virt_address = 0xC00000,
	.map_page     = VMM_umap_page
};

static int __mm_init(size_t bytes, malloc_head_t* head) {
	head->total_pages = bytes / PAGE_SIZE;
	if (bytes % PAGE_SIZE > 0) head->total_pages++;

	head->phys_address = (uint32_t)PMM_allocate_blocks(head->total_pages);
	head->list_head = (malloc_block_t*)head->virt_address;
	assert(head->phys_address);

	for (uint32_t i = 0, virt = head->virt_address; i < head->total_pages; i++, virt += PAGE_SIZE) {
		head->map_page((void*)(head->phys_address + i * PAGE_SIZE), (void*)virt);
		pt_entry_t* page = VMM_get_page(virt);
		SET_ATTRIBUTE(page, PTE_READ_WRITE);
	}

	if (head->list_head != NULL) {
		head->list_head->v_addr = head->phys_address;
		head->list_head->pcount = head->total_pages;
		head->list_head->size   = (head->total_pages * PAGE_SIZE) - sizeof(malloc_block_t);
		head->list_head->free   = 1;
		head->list_head->next   = NULL;
		return 1;
	}

	return -1;
}

static int _kmallocp(uint32_t v_addr) {
	return __kmallocp(v_addr, &_kernel_malloc);
}

static int _umallocp(uint32_t v_addr) {
	return __kmallocp(v_addr, &_user_malloc);
}

int ALC_mallocp(uint32_t v_addr, uint8_t type) {
	if (type == KERNEL) return _kmallocp(v_addr);
	else return _umallocp(v_addr);
}

// Memory allocation in kernel address space. Usermode will cause error
static void* _kmalloc(size_t size) {
	return __kmalloc(size, &_kernel_malloc, KERNEL);
}

static void* _umalloc(size_t size) {
	return __kmalloc(size, &_user_malloc, USER);
}

void* ALC_malloc(size_t size, uint8_t type) {
	if (type == KERNEL) return _kmalloc(size);
	else return _umalloc(size);
}

static void* _krealloc(void* ptr, size_t size) {
	return __krealloc(ptr, size, &_kernel_malloc, KERNEL);
}

static void* _urealloc(void* ptr, size_t size) {
	return __krealloc(ptr, size, &_user_malloc, USER);
}

void* ALC_realloc(void* ptr, size_t size, uint8_t type) {
	if (type == KERNEL) return _krealloc(ptr, size);
	else return _urealloc(ptr, size);
}

static int _kfree(void* ptr) {
	return __kfree(ptr, &_kernel_malloc);
}

static int _ufree(void* ptr) {
	return __kfree(ptr, &_user_malloc);
}

int ALC_free(void* ptr, uint8_t type) {
	if (type == KERNEL) return _kfree(ptr);
	else return _ufree(ptr);
}

static int _kfreep(void* v_addr) {
	pt_entry_t* page = VMM_get_page((v_addr_t)v_addr);
	if (PAGE_PHYS_ADDRESS(page) && TEST_ATTRIBUTE(page, PTE_PRESENT)) {
		VMM_free_page(page);
		VMM_unmap_page((uint32_t*)v_addr);
	}

	return 1;
}

int ALC_freep(void* v_addr, uint8_t type) {
	return _kfreep(v_addr);
}

static int __kmallocp(uint32_t virt, malloc_head_t* head) {
	uint32_t* phys = PMM_allocate_blocks(1);
	if (!phys) return -1;
	head->map_page((void*)phys, (void*)virt);
	return 1;
}

static void* __krealloc(void* ptr, size_t size, malloc_head_t* head, uint8_t type) {
	void* new_data = NULL;
	if (size) {
		if(!ptr) return __kmalloc(size, head, type);
		new_data = __kmalloc(size, head, type);
		if(new_data) {
			memcpy(new_data, ptr, size);
			__kfree(ptr, head);
		}
	}

	return new_data;
}

static void* __kmalloc(size_t size, malloc_head_t* head, uint8_t type) {
	if (size <= 0) return NULL;
	if (head->list_head == NULL) __mm_init(size, head);

	__merge_free_blocks(head->list_head);
	malloc_block_t* cur = head->list_head;
	while (cur->next != NULL) {
		if (cur->free) {
			if (cur->size == size) break;
			if (cur->size > size + sizeof(malloc_block_t)) break;
		}
		
		cur = cur->next;
	}
	
	if (size == cur->size) cur->free = 0;
	else if (cur->size > size + sizeof(malloc_block_t)) __block_split(cur, size);
	else {
		uint8_t num_pages = 1;
		while (cur->size + num_pages * PAGE_SIZE < size + sizeof(malloc_block_t))
			num_pages++;

		uint32_t virt = head->virt_address + head->total_pages * PAGE_SIZE; // TODO: new pages to new blocks. Don`t mix them to avoid pagedir errors in contswitch
		for (uint8_t i = 0; i < num_pages; i++) {
			__kmallocp(virt, head);

			virt += PAGE_SIZE;
			cur->size += PAGE_SIZE;
			head->total_pages++;
		}

		__block_split(cur, size);
	}

	return (void*)cur + sizeof(malloc_block_t);
}

static int __kfree(void* ptr, malloc_head_t* head) {
	if (!ptr) return -1;
	for (malloc_block_t* cur = head->list_head; cur->next; cur = cur->next) 
		if ((void*)cur + sizeof(malloc_block_t) == ptr && !cur->free) {
			cur->free = 1;
			memset(ptr, 0, cur->size);
			__merge_free_blocks(head->list_head);

			break;
		}

	for (malloc_block_t* cur = head->list_head; cur->next; cur = cur->next) {
		if ((void*)cur + sizeof(malloc_block_t) == ptr && !cur->free) {
			uint32_t num_pages = cur->pcount;
			for (uint32_t i = 0; i < num_pages; i++) {
				uint32_t v_addr = cur->v_addr + i * PAGE_SIZE;
				_kfreep((void*)v_addr);
			}

			// Mark the block as free and clear memory content
			cur->free = 1;
			memset(ptr, 0, cur->size);

			// Merge adjacent free blocks
			__merge_free_blocks(head->list_head);
			break;
		}
	}

	return 1;
}

static int __block_split(malloc_block_t* node, size_t size) {
	malloc_block_t* new_node = (malloc_block_t*)((void*)node + size + sizeof(malloc_block_t));

	new_node->size   = node->size - size - sizeof(malloc_block_t);
	new_node->free   = 1;
	new_node->next   = node->next;
	new_node->v_addr = node->v_addr;
	node->size       = size;
	node->free       = 0;
	node->next       = new_node;
	node->pcount     -= (size / PAGE_SIZE) + 1;

	return 1;
}

static int __merge_free_blocks(malloc_block_t* block) {
	malloc_block_t* cur = block;
	while (cur != NULL && cur->next != NULL) {
		if (cur->free && cur->next->free) {
			cur->size += (cur->next->size) + sizeof(malloc_block_t);
			if (cur->next->next != NULL) cur->next = cur->next->next;
			else {
				cur->next = NULL;
				break;
			}
		}

		cur = cur->next;
	}

	return 1;
}

static int _print_malloc(malloc_head_t* head) {
	kprintf(
		"\n|%i(%c)|", head->list_head->size + sizeof(malloc_block_t),
		head->list_head->free ? 'F' : 'O'
	);

	uint32_t total_free = head->list_head->size + sizeof(malloc_block_t);
	for (malloc_block_t* cur = head->list_head; cur->next; cur = cur->next) {
		if (cur->next != NULL) {
			total_free += cur->next->size + sizeof(malloc_block_t);
			kprintf(
				"%i(%c)|", cur->next->size + sizeof(malloc_block_t),
				cur->next->free ? 'F' : 'O'
			);
		}
	}

	kprintf(" TOTAL: [%iB]\n", total_free);
	return 1;	
}

int kprint_kmalloc() {
	_print_malloc(&_kernel_malloc);
	return 1;
}

int kprint_umalloc() {
	_print_malloc(&_user_malloc);
	return 1;
}

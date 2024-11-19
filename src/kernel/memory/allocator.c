#include "../include/allocator.h"


//===========================
//	GLOBAL VARS
//===========================

	malloc_block_t* kmalloc_list_head = NULL;
	malloc_block_t* umalloc_list_head = NULL;

	uint32_t malloc_virt_address  = 0x300000;
	uint32_t kmalloc_phys_address = 0;
	uint32_t umalloc_phys_address = 0;

	uint32_t total_kmalloc_pages = 0;
	uint32_t total_umalloc_pages = 0;

	page_directory* kmalloc_dir	= 0;
	page_directory* umalloc_dir = 0;

//===========================
//	GLOBAL VARS
//===========================
//	INITIALIZATION
//	- Allocate page if needed
//	- Create first malloc block
//===========================

	void kmm_init(const uint32_t bytes) {
		kmalloc_dir = current_page_directory;
		total_kmalloc_pages = bytes / PAGE_SIZE;
		if (bytes % PAGE_SIZE > 0) total_kmalloc_pages++;

		kmalloc_phys_address = (uint32_t)allocate_blocks(total_kmalloc_pages);
		kmalloc_list_head    = (malloc_block_t*)malloc_virt_address;
		assert(kmalloc_phys_address);

		for (uint32_t i = 0, virt = malloc_virt_address; i < total_kmalloc_pages; i++, virt += PAGE_SIZE) {
			map_page2kernel((void*)(kmalloc_phys_address + i * PAGE_SIZE), (void*)virt);
			pt_entry* page = get_page(virt);
			SET_ATTRIBUTE(page, PTE_READ_WRITE);
		}

		if (kmalloc_list_head != NULL) {
			kmalloc_list_head->v_addr = malloc_virt_address;
			kmalloc_list_head->pcount = total_kmalloc_pages;
			kmalloc_list_head->size   = (total_kmalloc_pages * PAGE_SIZE) - sizeof(malloc_block_t);
			kmalloc_list_head->free   = true;
			kmalloc_list_head->next   = NULL;
		}
	}

	void umm_init(const uint32_t bytes) {
		umalloc_dir = current_page_directory;
		total_umalloc_pages = bytes / PAGE_SIZE;
		if (bytes % PAGE_SIZE > 0) total_umalloc_pages++;

		umalloc_phys_address = (uint32_t)allocate_blocks(total_umalloc_pages);
		umalloc_list_head    = (malloc_block_t*)malloc_virt_address;
		assert(umalloc_phys_address);

		for (uint32_t i = 0, virt = malloc_virt_address; i < total_umalloc_pages; i++, virt += PAGE_SIZE) {
			map_page2user((void*)(umalloc_phys_address + i * PAGE_SIZE), (void*)virt);
			pt_entry* page = get_page(virt);
			SET_ATTRIBUTE(page, PTE_READ_WRITE);
		}

		if (umalloc_list_head != NULL) {
			umalloc_list_head->v_addr = malloc_virt_address;
			umalloc_list_head->pcount = total_umalloc_pages;
			umalloc_list_head->size   = (total_umalloc_pages * PAGE_SIZE) - sizeof(malloc_block_t);
			umalloc_list_head->free   = true;
			umalloc_list_head->next   = NULL;
		}
	}

//===========================
//	INITIALIZATION
//===========================
//	ALLOC FUNCTIONS
//	- Splitting for splitting malloc blocks
//	- KMalloc for allocating space
//	- Merge free blocks for merging blocks in page
//===========================

	void block_split(malloc_block_t* node, const uint32_t size) {
		malloc_block_t* new_node = (malloc_block_t*)((void*)node + size + sizeof(malloc_block_t));

		new_node->size   = node->size - size - sizeof(malloc_block_t);
		new_node->free   = true;
		new_node->next   = node->next;
		new_node->v_addr = node->v_addr;

		node->size   = size;
		node->free   = false;
		node->next   = new_node;
		node->pcount -= (size / PAGE_SIZE) + 1;
	}
	
	void* kmallocp(uint32_t v_addr, uint8_t type) {
		if (type == KERNEL) map_page2kernel((void*)allocate_blocks(1), (void*)v_addr);
		else map_page2user((void*)allocate_blocks(1), (void*)v_addr);
	}

	// Memory allocation in kernel address space. Usermode will cause error
	void* kmalloc(malloc_block_t* entry, const uint32_t size, uint8_t type) {
		if (size <= 0) return 0;
		if (kmalloc_list_head == NULL) kmm_init(size);

		//=============
		// Find a block
		//=============

			merge_free_blocks(kmalloc_list_head);
			malloc_block_t* cur = kmalloc_list_head;
			while (cur->next != NULL) {
				if (cur->free == true) {
					if (cur->size == size) break;
					if (cur->size > size + sizeof(malloc_block_t)) break;
				}
				
				cur = cur->next;
			}

		//=============
		// Find a block
		//=============
		// Work with block
		//=============
		
			if (size == cur->size) cur->free = false;
			else if (cur->size > size + sizeof(malloc_block_t)) block_split(cur, size);
			else {
				//=============
				// Allocate new page
				//=============
				
					uint8_t num_pages = 1;
					while (cur->size + num_pages * PAGE_SIZE < size + sizeof(malloc_block_t))
						num_pages++;

					uint32_t virt = malloc_virt_address + total_kmalloc_pages * PAGE_SIZE; // TODO: new pages to new blocks. Don`t mix them to avoid pagedir errors in contswitch
					for (uint8_t i = 0; i < num_pages; i++) {
						kmallocp(virt, type);

						virt += PAGE_SIZE;
						cur->size += PAGE_SIZE;
						total_kmalloc_pages++;
					}

					block_split(cur, size);

				//=============
				// Allocate new page
				//=============
			}
		
		//=============
		// Work with block
		//=============

		return (void*)cur + sizeof(malloc_block_t);
	}

	void* krealloc(void* ptr, size_t size) {
		void* new_data = NULL;
		if (size) {
			if(!ptr) return kmalloc(kmalloc_list_head, size, KERNEL);

			new_data = kmalloc(kmalloc_list_head, size, KERNEL);
			if(new_data) {
				memcpy(new_data, ptr, size);
				kfree(kmalloc_list_head, ptr);
			}
		}

		return new_data;
	}

	void merge_free_blocks(malloc_block_t* block) {
		malloc_block_t* cur = block;
		while (cur != NULL && cur->next != NULL) {
			if (cur->free == true && cur->next->free == true) {
				cur->size += (cur->next->size) + sizeof(malloc_block_t);
				if (cur->next->next != NULL) cur->next = cur->next->next;
				else {
					cur->next = NULL;
					break;
				}
			}

			cur = cur->next;
		}
	}

	void kfree(malloc_block_t* entry, void* ptr) {
		if (ptr == NULL) return;
		for (malloc_block_t* cur = entry; cur->next; cur = cur->next) 
			if ((void*)cur + sizeof(malloc_block_t) == ptr && cur->free == false) {
				cur->free = true;
				memset(ptr, 0, cur->size);
				merge_free_blocks(entry);

				break;
			}

		cleanup(entry, ptr);
	}

	void cleanup(malloc_block_t* entry, void* ptr) {
		for (malloc_block_t* cur = entry; cur->next; cur = cur->next) {
			if ((void*)cur + sizeof(malloc_block_t) == ptr && cur->free == false) {
				uint32_t num_pages = cur->pcount;
				for (uint32_t i = 0; i < num_pages; i++) {
					uint32_t v_addr = cur->v_addr + i * PAGE_SIZE;
					kfreep((void*)v_addr);
				}

				// Mark the block as free and clear memory content
				cur->free = true;
				memset(ptr, 0, cur->size);

				// Merge adjacent free blocks
				merge_free_blocks(entry);
				break;
			}
		}
	}

	void kfreep(void* v_addr) {
		pt_entry* page = get_page((virtual_address)v_addr);
		if (PAGE_PHYS_ADDRESS(page) && TEST_ATTRIBUTE(page, PTE_PRESENT)) {
			free_page(page);
			unmap_page((uint32_t*)v_addr);
			flush_tlb_entry((virtual_address)v_addr);
		}
	}

//===========================
//	ALLOC FUNCTIONS
//===========================
//	INFO
//===========================

	uint32_t kmalloc_total_free() {
		uint32_t total_free = 0;
		if (kmalloc_list_head->free = true) total_free += kmalloc_list_head->size + sizeof(malloc_block_t);
		for (malloc_block_t* cur = kmalloc_list_head; cur->next; cur = cur->next)
			if (cur->next != NULL)
				if (cur->next->free == true) total_free += cur->next->size + sizeof(malloc_block_t);
		
		return total_free;
	}

	uint32_t kmalloc_total_avaliable() {
		uint32_t total_free = kmalloc_list_head->size + sizeof(malloc_block_t);
		for (malloc_block_t* cur = kmalloc_list_head; cur->next; cur = cur->next)
			if (cur->next != NULL)
				total_free += cur->next->size + sizeof(malloc_block_t);
		
		return total_free;
	}

	uint32_t umalloc_total_avaliable() {
		uint32_t total_free = umalloc_list_head->size + sizeof(malloc_block_t);
		for (malloc_block_t* cur = umalloc_list_head; cur->next; cur = cur->next)
			if (cur->next != NULL)
				total_free += cur->next->size + sizeof(malloc_block_t);
		
		return total_free;
	}

	void print_malloc_map() {
#ifndef USERMODE 

		kprintf(
			"\n|%i(%c)|",
			kmalloc_list_head->size + sizeof(malloc_block_t),
			kmalloc_list_head->free == true ? 'F' : 'O'
		);

		for (malloc_block_t* cur = kmalloc_list_head; cur->next; cur = cur->next)
			if (cur->next != NULL)
				kprintf(
					"%i(%c)|",
					cur->next->size + sizeof(malloc_block_t),
					cur->next->free == true ? 'F' : 'O'
				);

		kprintf(" TOTAL: [%iB]\n", kmalloc_total_avaliable());

#elif defined(USERMODE)

		kprintf(
			"\n|%i(%c)|",
			umalloc_list_head->size + sizeof(malloc_block_t),
			umalloc_list_head->free == true ? 'F' : 'O'
		);

		for (malloc_block_t* cur = umalloc_list_head; cur->next; cur = cur->next)
			if (cur->next != NULL)
				kprintf(
					"%i(%c)|",
					cur->next->size + sizeof(malloc_block_t),
					cur->next->free == true ? 'F' : 'O'
				);

		kprintf(" TOTAL: [%iB]\n", umalloc_total_avaliable());

#endif
	}

//===========================
//	INFO
//===========================
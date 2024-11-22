#include "../include/vmm.h"


page_directory* current_page_directory;
page_directory* kernel_page_directory;


bool VMM_init(uint32_t memory_start) {
    page_directory* dir = _mkpdir();
    page_table* table3G = (page_table*)PMM_allocate_blocks(1);
    if (table3G == NULL) return false;

    memset(table3G, 0, sizeof(page_table));
    for (uint32_t i = 0, frame = 0x0; i < PAGES_PER_TABLE; i++, frame += PAGE_SIZE) {
        pt_entry page = 0;
        SET_ATTRIBUTE(&page, PTE_PRESENT);
        SET_ATTRIBUTE(&page, PTE_READ_WRITE);
        SET_FRAME(&page, frame);

        table3G->entries[i] = page;
    }

    pd_entry* entry = &dir->entries[0];
    SET_ATTRIBUTE(entry, PDE_PRESENT);
    SET_ATTRIBUTE(entry, PDE_READ_WRITE);
    SET_FRAME(entry, (physical_address)table3G); 

    page_table* table = (page_table*)PMM_allocate_blocks(1);
    memset(table, 0, sizeof(page_table));
    if (table == NULL) return false;

    for (uint32_t i = 0, frame = memory_start; i < PAGES_PER_TABLE; i++, frame += PAGE_SIZE) {
        pt_entry page = 0;
        SET_ATTRIBUTE(&page, PTE_PRESENT);
        SET_ATTRIBUTE(&page, PTE_READ_WRITE);
        SET_FRAME(&page, frame);

        table->entries[i] = page;
    }
    
    entry = &dir->entries[PD_INDEX(0xC0000000)];
    SET_ATTRIBUTE(entry, PDE_PRESENT);
    SET_ATTRIBUTE(entry, PDE_READ_WRITE);
    SET_FRAME(entry, (uint32_t)table);

    if (VMM_set_directory(dir) == false) return false;
    kernel_page_directory = dir;

	uint32_t cr0;
	asm ("mov %%cr0, %0": "=r"(cr0));
	cr0 |= 0x80000000;
	asm ("mov %0, %%cr0":: "r"(cr0));

    i386_isr_registerHandler(14, _page_fault);
    return true;
}

bool VMM_set_directory(page_directory* pd) {
    if (!pd) {
        kprintf("[%s %i] Can`t set page directory!\n", __FILE__, __LINE__);
        return false;
    }

    current_page_directory = pd;
    asm ("mov %0, %%cr3":: "r"(current_page_directory));
    return true;
}

void* VMM_allocate_page(pt_entry* page) {
    void* block = PMM_allocate_blocks(1);
    if (block) {
        SET_FRAME(page, (physical_address)block);
        SET_ATTRIBUTE(page, PTE_PRESENT);
    } 
    else {
        kprintf("[%s %i] Page allocation error!\n", __FILE__, __LINE__);
        return NULL;
    }
    
    return block;
}

pt_entry* VMM_get_page(const virtual_address address) {
    page_directory* pd = current_page_directory; 
    pd_entry* entry = &pd->entries[PD_INDEX(address)];
    page_table* table = (page_table*)PAGE_PHYS_ADDRESS(entry);
    return &table->entries[PT_INDEX(address)];
}

void VMM_free_page(pt_entry* page) {
    void* address = (void*)PAGE_PHYS_ADDRESS(page);
    if (address) PMM_free_blocks((uint32_t*)address, 1);
    CLEAR_ATTRIBUTE(page, PTE_PRESENT);
}

bool VMM_kmap_page(void* phys_address, void* virt_address) {
    page_directory* pd = current_page_directory;
    pd_entry* entry = &pd->entries[PD_INDEX((uint32_t)virt_address)];

    if ((*entry & PTE_PRESENT) != PTE_PRESENT) {
        page_table* table = (page_table*)PMM_allocate_blocks(1);
        if (!table) return false;

        memset(table, 0, sizeof(page_table));
        pd_entry* entry = &pd->entries[PD_INDEX((uint32_t)virt_address)];

        SET_ATTRIBUTE(entry, PDE_PRESENT);
        SET_ATTRIBUTE(entry, PDE_READ_WRITE);
        SET_FRAME(entry, (physical_address)table);
    }

    page_table* table = (page_table*)PAGE_PHYS_ADDRESS(entry);
    pt_entry* page = &table->entries[PT_INDEX((uint32_t)virt_address)];

    SET_ATTRIBUTE(page, PTE_PRESENT);
    SET_FRAME(page, (uint32_t)phys_address);
    return true;
}

bool VMM_umap_page(void* phys_address, void* virt_address) {
    page_directory* pd = current_page_directory;
    pd_entry* entry = &pd->entries[PD_INDEX((uint32_t)virt_address)];

    if ((*entry & PTE_PRESENT) != PTE_PRESENT) {
        page_table* table = (page_table*)PMM_allocate_blocks(1);
        if (!table) return false;

        memset(table, 0, sizeof(page_table));
        pd_entry* entry = &pd->entries[PD_INDEX((uint32_t)virt_address)];

        SET_ATTRIBUTE(entry, PDE_PRESENT);
        SET_ATTRIBUTE(entry, PDE_USER); // Set user mode permissions
        SET_ATTRIBUTE(entry, PDE_READ_WRITE); // Adjust permissions as needed
        SET_FRAME(entry, (physical_address)table);
    }

    page_table* table = (page_table*)PAGE_PHYS_ADDRESS(entry);
    pt_entry* page = &table->entries[PT_INDEX((uint32_t)virt_address)];

    SET_ATTRIBUTE(page, PTE_PRESENT);
    SET_ATTRIBUTE(page, PTE_USER); // Set user mode permissions
    SET_FRAME(page, (uint32_t)phys_address);
    return true;
}

void VMM_unmap_page(void* virt_address) {
    pt_entry* page = VMM_get_page((uint32_t)virt_address);
    SET_FRAME(page, 0);
    CLEAR_ATTRIBUTE(page, PTE_PRESENT);
}

physical_address VMM_virtual2physical(void* virt_address) {
    page_directory* pd = current_page_directory;
    pd_entry* pd_entry = &pd->entries[PD_INDEX((uint32_t)virt_address)];
    if ((*pd_entry & PTE_PRESENT) != PTE_PRESENT) return 0;

    page_table* pt     = (page_table*)PAGE_PHYS_ADDRESS(pd_entry);
    pt_entry* pt_entry = &pt->entries[PT_INDEX((uint32_t)virt_address)];
    if ((*pt_entry & PTE_PRESENT) != PTE_PRESENT) return 0;

    physical_address phys_address = PAGE_PHYS_ADDRESS(pt_entry) | OFFSET_IN_PAGE((uint32_t)virt_address);
    return phys_address;
}

void _flush_tlb_entry(virtual_address address) {
    asm ("cli; invlpg (%0); sti" : : "r"(address) );
}

page_directory* _mkpdir() {
    page_directory* dir = (page_directory*)PMM_allocate_blocks(3);
    if (dir == NULL) {
        kprintf("[%s %i] Failed to allocate memory for page directory\n",  __FILE__, __LINE__);
        return NULL;
    }

    memset(dir, 0, sizeof(page_directory));
    for (uint32_t i = 0; i < TABLES_PER_DIRECTORY; i++)
        dir->entries[i] = 0x02;

    return dir;
}

page_directory* _mkupdir() {
    page_directory* user_page_directory = _mkpdir();
    page_table* user_table = (page_table*)PMM_allocate_blocks(1);
    if (user_table == NULL) {
        kprintf("[%s %i] Failed to allocate memory for user page table\n",  __FILE__, __LINE__);
        PMM_free_blocks((uint32_t*)user_page_directory, 3);
        return NULL;
    }

    memset(user_table, 0, sizeof(page_table));
    for (uint32_t i = 0, frame = USER_MEMORY_START; i < USER_PAGES; i++, frame += PAGE_SIZE) {
        pt_entry page = 0;
        SET_ATTRIBUTE(&page, PTE_PRESENT);
        SET_ATTRIBUTE(&page, PTE_READ_WRITE);
        SET_ATTRIBUTE(&page, PTE_USER);
        SET_FRAME(&page, frame);

        user_table->entries[PT_INDEX(frame)] = page;
    }

    pd_entry* user_entry = &user_page_directory->entries[USER_TABLE_INDEX];
    SET_ATTRIBUTE(user_entry, PDE_PRESENT);
    SET_ATTRIBUTE(user_entry, PDE_READ_WRITE);
    SET_ATTRIBUTE(user_entry, PDE_USER);
    SET_FRAME(user_entry, (uint32_t)user_table);

    return user_page_directory;
}

void _free_pdir(page_directory* pd) {
    if (pd == NULL) {
        kprintf("[%s %i] Error: Attempting to free a NULL page directory.\n", __FILE__, __LINE__);
        return;
    }

    for (int pd_index = 0; pd_index < TABLES_PER_DIRECTORY; pd_index++) {
        pd_entry* pd_entry = &pd->entries[pd_index];

        if ((*pd_entry & PDE_PRESENT) == PDE_PRESENT) {
            page_table* pt = (page_table*)PAGE_PHYS_ADDRESS(pd_entry);

            for (int pt_index = 0; pt_index < PAGES_PER_TABLE; pt_index++) {
                pt_entry* page = &pt->entries[pt_index];

                if (*page & PTE_PRESENT) {
                    void* phys_address = (void*)PAGE_PHYS_ADDRESS(page);
                    PMM_free_blocks((uint32_t*)phys_address, 1);
                }
            }

            PMM_free_blocks((uint32_t*)pt, 1);
        }
    }

    PMM_free_blocks((uint32_t*)pd, 3);
}

void _copy_dir2dir(page_directory* src, page_directory* dest) {
    if (!src || !dest) return;
    for (uint32_t i = 0; i < TABLES_PER_DIRECTORY; i++) {
        if (src->entries[i] & PDE_PRESENT) {
            page_table* new_table = (page_table*)PMM_allocate_blocks(1);
            if (!new_table) {
                PMM_free_blocks((uint32_t*)dest, 3);
                return;
            }

            memcpy(new_table, (page_table*)PAGE_PHYS_ADDRESS(&src->entries[i]), sizeof(page_table));
            dest->entries[i] = (pd_entry)((uint32_t)new_table | PDE_PRESENT | PDE_READ_WRITE);
        }
    }
}

void _page_fault(struct Registers* regs) {
    kclrscr();

	uint32_t faulting_address = 0;
	asm ("mov %%cr2, %0" : "=r" (faulting_address));

	int present	 = !(regs->error & 0x1);	// When set, the page fault was caused by a page-protection violation. When not set, it was caused by a non-present page.
	int rw		 = regs->error & 0x2;		// When set, the page fault was caused by a write access. When not set, it was caused by a read access.
	int us		 = regs->error & 0x4;		// When set, the page fault was caused while CPL = 3. This does not necessarily mean that the page fault was a privilege violation.
	int reserved = regs->error & 0x8;		// When set, one or more page directory entries contain reserved bits which are set to 1. This only applies when the PSE or PAE flags in CR4 are set to 1.
	int id		 = regs->error & 0x10;		// When set, the page fault was caused by an instruction fetch. This only applies when the No-Execute bit is supported and enabled.

	kprintf("\nWHOOOPS..\nPAGE FAULT! (\t");

    //=======
    // PARAMS

        if (present) kprintf("NOT PRESENT\t");
        else kprintf("PAGE PROTECTION\t");
        
        if (rw) kprintf("READONLY\t");
        else kprintf("WRITEONLY\t");

        if (us) kprintf("USERMODE\t");
        if (reserved) kprintf("RESERVED\t");
        if (id) kprintf("INST FETCH\t");

    //
    //=======

	kprintf(") AT 0x%p\n", faulting_address);
    kprintf("CHECK YOUR CODE, BUDDY!\n");
    kprintf("\nSTACK TRACE:\n");

    i386_isr_interrupt_details(faulting_address, regs->ebp, regs->esp);

	kernel_panic("\nPAGE FAULT");
}
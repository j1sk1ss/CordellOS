#include <vmm.h>

static directories_t _dirs = { .curr = NULL, .kern = NULL };
directories_t* VMM_get_dirs() {
    return &_dirs;
}

int VMM_set_directory(pdir_t* pd) {
    if (!pd) return false;
    _dirs.curr = pd;
    asm("mov %0, %%cr3":: "r"(_dirs.curr));
    return true;
}

pdir_t* VMM_mkpdir() {
    pdir_t* dir = (pdir_t*)PMM_allocate_blocks(1);
    if (!dir) return NULL;
    memset(dir, 0, sizeof(pdir_t));
    for (uint32_t i = 0; i < TABLES_PER_DIRECTORY; i++) {
        dir->entries[i] = 0x02;
    }

    return dir;
}

void VMM_free_pdir(pdir_t* pd) {
    if (!pd) return;
    for (int pd_index = 0; pd_index < TABLES_PER_DIRECTORY; pd_index++) {
        pd_entry_t* pd_entry_t = &pd->entries[pd_index];
        if ((*pd_entry_t & PDE_PRESENT) == PDE_PRESENT) 
            VMM_free_table((ptable_t*)PAGE_PHYS_ADDRESS(pd_entry_t));
    }

    PMM_free_blocks((uint32_t*)pd, 1);
}

void VMM_copy_dir2dir(pdir_t* src, pdir_t* dest) {
    if (!src || !dest) return;
    for (uint32_t i = 0; i < TABLES_PER_DIRECTORY; i++) {
        if (src->entries[i] & PDE_PRESENT) {
            ptable_t* tb = (ptable_t*)PMM_allocate_blocks(1);
            if (!tb) {
                VMM_free_pdir(dest);
                return;
            }

            memcpy(tb, (ptable_t*)PAGE_PHYS_ADDRESS(&src->entries[i]), sizeof(ptable_t));
            dest->entries[i] = (pd_entry_t)((uint32_t)tb | PDE_PRESENT | PDE_READ_WRITE);
        }
    }
}

uint32_t VMM_mkpage(p_addr_t p_addr, uint8_t type) {
    pt_entry_t page = 0;
    SET_ATTRIBUTE(&page, PTE_PRESENT);
    if (type == USER) { SET_ATTRIBUTE(&page, PTE_USER); }
    SET_ATTRIBUTE(&page, PTE_READ_WRITE);
    SET_FRAME(&page, p_addr);
    return page;
}

pt_entry_t* VMM_get_page(const v_addr_t address) {
    pdir_t* pd = _dirs.curr; 
    pd_entry_t* entry = &pd->entries[PD_INDEX(address)];
    ptable_t* table = (ptable_t*)PAGE_PHYS_ADDRESS(entry);
    return &table->entries[PT_INDEX(address)];
}

void VMM_free_page(pt_entry_t* page) {
    void* address = (void*)PAGE_PHYS_ADDRESS(page);
    if (address) PMM_free_blocks((uint32_t*)address, 1);
    CLEAR_ATTRIBUTE(page, PTE_PRESENT);
}

void VMM_unmap_page(void* virt_address) {
    pt_entry_t* page = VMM_get_page((uint32_t)virt_address);
    SET_FRAME(page, 0);
    CLEAR_ATTRIBUTE(page, PTE_PRESENT);
}

ptable_t* VMM_mkptable(uint32_t p_addr, uint8_t type) {
    ptable_t* table = (ptable_t*)PMM_allocate_blocks(1);
    if (table == NULL) return NULL;

    memset(table, 0, sizeof(ptable_t));
    // Fill table with pages PTE_PRESENT | PRE_READ_WRITE
    for (uint32_t i = 0, frame = p_addr; i < PAGES_PER_TABLE; i++, frame += PAGE_SIZE) {
        table->entries[i] = VMM_mkpage(frame, type);
    }

    return table;
}

static int _map_table(pdir_t* pd, ptable_t* table, uint8_t type, size_t index) {
    if (!pd || !table) return 0;
    pd_entry_t* entry = &pd->entries[index];
    SET_ATTRIBUTE(entry, PDE_PRESENT);
    SET_ATTRIBUTE(entry, PDE_READ_WRITE);
    if (type == USER) { SET_ATTRIBUTE(entry, PDE_USER); }
    SET_FRAME(entry, (uint32_t)table);
    return 1;
}

static int _map_page(void* p_addr, void* v_addr, uint8_t type) {
    pdir_t* pd = _dirs.curr;
    pd_entry_t* entry = &pd->entries[PD_INDEX((v_addr_t)v_addr)];
    if ((*entry & PDE_PRESENT) != PDE_PRESENT) {
        ptable_t* table = (ptable_t*)PMM_allocate_blocks(1);
        if (!table) return 0;

        memset(table, 0, sizeof(ptable_t));
        if (!_map_table(pd, table, type, PD_INDEX((v_addr_t)v_addr))) {
            PMM_free_blocks((uint32_t*)table, 1);
            return 0;
        }
    }

    SET_ATTRIBUTE(entry, PDE_READ_WRITE);
    if (type == USER) { SET_ATTRIBUTE(entry, PDE_USER); }

    ptable_t* table = (ptable_t*)PAGE_PHYS_ADDRESS(entry);
    pt_entry_t* page = &table->entries[PT_INDEX((v_addr_t)v_addr)];

    SET_ATTRIBUTE(page, PTE_PRESENT);
    SET_ATTRIBUTE(page, PTE_READ_WRITE);
    if (type == USER) { SET_ATTRIBUTE(page, PTE_USER); }
    SET_FRAME(page, (p_addr_t)p_addr);
    return 1;    
}

int VMM_kmap_page(void* phys_address, void* virt_address) {
    return _map_page(phys_address, virt_address, KERNEL);
}

int VMM_umap_page(void* phys_address, void* virt_address) {
    return _map_page(phys_address, virt_address, USER);
}

void VMM_free_table(ptable_t* table) {
    for (int pt_index = 0; pt_index < PAGES_PER_TABLE; pt_index++) {
        pt_entry_t* page = &table->entries[pt_index];
        if (*page & PTE_PRESENT) {
            VMM_free_page(page);
        }
    }

    PMM_free_blocks((uint32_t*)table, 1);
}

p_addr_t VMM_virtual2physical(void* v_addr) {
    pdir_t* pd = _dirs.curr;
    pd_entry_t* pd_entry_t = &pd->entries[PD_INDEX((v_addr_t)v_addr)];
    if ((*pd_entry_t & PTE_PRESENT) != PTE_PRESENT) return 0;

    ptable_t* pt = (ptable_t*)PAGE_PHYS_ADDRESS(pd_entry_t);
    pt_entry_t* pt_entry_t = &pt->entries[PT_INDEX((v_addr_t)v_addr)];
    if ((*pt_entry_t & PTE_PRESENT) != PTE_PRESENT) return 0;

    return PAGE_PHYS_ADDRESS(pt_entry_t) | OFFSET_IN_PAGE((v_addr_t)v_addr);
}

static void _page_fault(struct Registers* regs) {
    kclrscr();

    uint32_t faulting_address = 0;
    asm ("mov %%cr2, %0" : "=r" (faulting_address));

    int not_present = !(regs->error & 0x1); // When set, the page fault was caused by a page-protection violation. When not set, it was caused by a non-present page.
    int write	     = regs->error & 0x2;	// When set, the page fault was caused by a write access. When not set, it was caused by a read access.
    int us		 = regs->error & 0x4;	    // When set, the page fault was caused while CPL = 3. This does not necessarily mean that the page fault was a privilege violation.
    int reserved = regs->error & 0x8;	    // When set, one or more page directory entries contain reserved bits which are set to 1. This only applies when the PSE or PAE flags in CR4 are set to 1.
    int id		 = regs->error & 0x10;	    // When set, the page fault was caused by an instruction fetch. This only applies when the No-Execute bit is supported and enabled.

    kprintf("\nWHOOOPS..\nPAGE FAULT! (\t");
    if (not_present) kprintf("NOT PRESENT\t"); else kprintf("PAGE PROTECTION\t");
    if (write)       kprintf("WRITE\t");       else kprintf("READ\t");
    if (us)          kprintf("USERMODE\t");
    if (reserved)    kprintf("RESERVED\t");
    if (id)          kprintf("INST FETCH\t");
    kprintf(") AT 0x%p\n", faulting_address);
    kprintf("EIP=0x%p ESP=0x%p EBP=0x%p ERROR=0x%p\n", regs->eip, regs->esp, regs->ebp, regs->error);
    kprintf("CHECK YOUR CODE, BUDDY!\n");
    kprintf("\nSTACK TRACE:\n");

    i386_isr_interrupt_details(regs->eip, regs->ebp, regs->esp);
    kernel_panic("PAGE FAULT");
}

int VMM_init(uint32_t memory_start) {
    pdir_t* dir = VMM_mkpdir();
    if (!dir) return 0;

    uint64_t identity_limit64 = (uint64_t)PMM_map.max_blocks * BLOCK_SIZE;
    if (identity_limit64 > USER_MEMORY_START) identity_limit64 = USER_MEMORY_START;

    for (uint32_t addr = 0; addr < (uint32_t)identity_limit64; addr += PAGES_PER_TABLE * PAGE_SIZE) {
        if (!_map_table(dir, VMM_mkptable(addr, KERNEL), KERNEL, PD_INDEX(addr))) {
            return 0;
        }
    }

    if (!_map_table(dir, VMM_mkptable(memory_start, KERNEL), KERNEL, PD_INDEX(0xC0000000))) {
        return 0;
    }
    
    if (!VMM_set_directory(dir)) return 0;
    _dirs.kern = dir;

	uint32_t cr0 = 0;
	asm("mov %%cr0, %0": "=r"(cr0));
	cr0 |= 0x80000000;
	asm("mov %0, %%cr0":: "r"(cr0));

    i386_isr_register_handler(14, _page_fault);
    return 1;
}

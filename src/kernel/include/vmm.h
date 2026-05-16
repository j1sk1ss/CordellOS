#ifndef VMM_H_
#define VMM_H_

#include <string.h>
#include <isr.h>
#include <pmm.h>

#define PAGES_PER_TABLE              1024
#define TABLES_PER_DIRECTORY         1024
#define PAGE_SIZE                    4096

#define USER_MEMORY_START            0xC0000000
#define USER_PAGES                   64
#define USER_TABLE_INDEX             1024

#define PD_INDEX(address)            ((address) >> 22)
#define PT_INDEX(address)            (((address) >> 12) & 0x3FF) // Max index 1023 = 0x3FF
#define PAGE_PHYS_ADDRESS(dir_entry) ((*dir_entry) & ~0xFFF)     // Clear lowest 12 bits, only return frame/address
#define SET_ATTRIBUTE(entry, attr)   (*entry |= attr)
#define CLEAR_ATTRIBUTE(entry, attr) (*entry &= ~attr)
#define TEST_ATTRIBUTE(entry, attr)  (*entry & attr)
#define SET_FRAME(entry, address)    (*entry = (*entry & ~0x7FFFF000) | address)   // Only set address/frame, not flags
#define OFFSET_IN_PAGE(address)      ((uint32_t)(address) & 0xFFF)

typedef uint32_t pt_entry_t;
typedef uint32_t pd_entry_t;
typedef uint32_t p_addr_t; 
typedef uint32_t v_addr_t; 

typedef enum {
    KERNEL = 0,
    USER   = 1
} user_t;

typedef enum {
    PTE_PRESENT       = 0x01,
    PTE_READ_WRITE    = 0x02,
    PTE_USER          = 0x04,
    PTE_WRITE_THROUGH = 0x08,
    PTE_CACHE_DISABLE = 0x10,
    PTE_ACCESSED      = 0x20,
    PTE_DIRTY         = 0x40,
    PTE_PAT           = 0x80,
    PTE_GLOBAL        = 0x100,
    PTE_FRAME         = 0x7FFFF000,   // bits 12+
} ptaccess_t;

typedef enum {
    PDE_PRESENT       = 0x01,
    PDE_READ_WRITE    = 0x02,
    PDE_USER          = 0x04,
    PDE_WRITE_THROUGH = 0x08,
    PDE_CACHE_DISABLE = 0x10,
    PDE_ACCESSED      = 0x20,
    PDE_DIRTY         = 0x40,          // 4MB entry only
    PDE_PAGE_SIZE     = 0x80,          // 0 = 4KB page, 1 = 4MB page
    PDE_GLOBAL        = 0x100,         // 4MB entry only
    PDE_PAT           = 0x2000,        // 4MB entry only
    PDE_FRAME         = 0x7FFFF000,    // bits 12+
} pdaccess_t;

typedef struct {
    pt_entry_t entries[PAGES_PER_TABLE];
} ptable_t;

typedef struct {
    pd_entry_t entries[TABLES_PER_DIRECTORY];
} pdir_t;

typedef struct {
    pdir_t* curr;
    pdir_t* kern;
} directories_t;

directories_t* VMM_get_dirs();
int VMM_init(uint32_t kernell_address);
pdir_t* VMM_mkpdir();
int VMM_set_directory(pdir_t* pd);
void VMM_free_pdir(pdir_t* pd);
void VMM_copy_dir2dir(pdir_t* src, pdir_t* dest);
ptable_t* VMM_mkptable(uint32_t p_addr, uint8_t type);
void VMM_free_table(ptable_t* table);
uint32_t VMM_mkpage(p_addr_t p_addr, uint8_t type);
pt_entry_t* VMM_get_page(const v_addr_t address);
void VMM_free_page(pt_entry_t* page);
int VMM_kmap_page(void* phys_address, void* virt_address);
int VMM_umap_page(void* phys_address, void* virt_address);
void VMM_unmap_page(void* virt_address);
p_addr_t VMM_virtual2physical(void* virt_address);

#endif
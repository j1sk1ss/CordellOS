#ifndef ELF_H_
#define ELF_H_


#include <stdint.h>
#include <stdbool.h>
#include <memory.h>
#include <math.h>
#include <fslib.h>

#include "elf.h"
#include "vfs.h"
#include "stdio.h"
#include "vmm.h"
#include "pmm.h"
#include "allocator.h"

#include "../multiboot/multiboot.h"


#define NULL_ADDRESS        -1
#define ELF32_ST_TYPE(i)    ((i)&0xf)
#define EI_NIDENT           (16)


typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t	  e_type;
    uint16_t	  e_machine;
    uint32_t	  e_version;
    uint32_t	  e_entry;
    uint32_t	  e_phoff;
    uint32_t	  e_shoff;
    uint32_t	  e_flags;
    uint16_t	  e_ehsize;
    uint16_t	  e_phentsize;
    uint16_t	  e_phnum;
    uint16_t	  e_shentsize;
    uint16_t	  e_shnum;
    uint16_t	  e_shstrndx;
} Elf32_Ehdr;

// e_type values
enum {
    ET_NONE = 0x0,
    ET_REL,
    ET_EXEC,
    ET_DYN,
    // ...
};

typedef struct {
    uint32_t	p_type;
    uint32_t	p_offset;
    uint32_t	p_vaddr;
    uint32_t	p_paddr;
    uint32_t	p_filesz;
    uint32_t	p_memsz;
    uint32_t	p_flags;
    uint32_t	p_align;
} Elf32_Phdr;

// p_type values
enum {
    PT_NULL = 0x0,
    PT_LOAD,            // Loadable section
    // ...
};

typedef struct Elf32_sectionHeader {
	uint32_t	sh_name;
	uint32_t	sh_type;
	uint32_t	sh_flags;
	uint32_t	sh_addr;
	uint32_t	sh_offset;
	uint32_t	sh_size;
	uint32_t	sh_link;
	uint32_t	sh_info;
	uint32_t	sh_addralign;
	uint32_t	sh_entsize;
} Elf32_Shdr;

typedef struct {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
} Elf32_Sym;

typedef struct {
    uint32_t* pages;
    uint32_t pages_count;
    void* entry_point;
} ELF32_program;

enum ShT_Types {
	SHT_NULL	 = 0,   // Null section
	SHT_PROGBITS = 1,   // Program information
	SHT_SYMTAB	 = 2,   // Symbol table
	SHT_STRTAB	 = 3,   // String table
	SHT_RELA	 = 4,   // Relocation (w/ addend)
	SHT_NOBITS	 = 8,   // Not present in file
	SHT_REL		 = 9,   // Relocation (no addend)
};
 
enum ShT_Attributes {
	SHF_WRITE	= 0x01, // Writable section
	SHF_ALLOC	= 0x02  // Exists in memory
};

typedef struct {
  uint32_t name_offset_in_strtab;
  uint32_t value;
  uint32_t size;
  uint8_t  info;
  uint8_t  other;
  uint16_t shndx;
} __attribute__((packed)) elf_symbol_t;

typedef struct {
  elf_symbol_t *symtab;
  uint32_t      symtab_size;
  const char   *strtab;
  uint32_t      strtab_size;
} elf_symbols_t;

typedef struct ELF32_symbols_desctiptor {
    bool present;
    uint32_t num_symbols;
    Elf32_Sym* symbols;
    char* string_table_addr;
} ELF32_SymDescriptor;


ELF32_program* ELF_read(const char* path, int type);
void ELF_free_program(ELF32_program* program, uint8_t type);

void ELF_kernel_trace();
void ELF_build_symbols_from_multiboot(multiboot_elf_section_header_table_t mb);
const char* ELF_lookup_symbol_function(uint32_t addr, elf_symbols_t* elf);
const char* ELF_lookup_function(uint32_t addr);


#endif
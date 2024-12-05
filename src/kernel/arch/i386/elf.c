// Thanks to: https://github.com/Jorl17/jOS/blob/master/elf.c#L61
//            https://github.com/makerimages/SwormOS/tree/master/kernel
#include "../../include/elf.h"


static elf_symbols_t kernel_elf_symbols;


void ELF_build_symbols_from_multiboot(multiboot_elf_section_header_table_t header) {
	Elf32_Shdr* sh = (Elf32_Shdr*)(header.addr);
	uint32_t shstrtab = sh[header.shndx].sh_addr;

	for (uint32_t i = 0; i < header.num; i++) {
		const char* name = (const char*) (shstrtab + sh[i].sh_name);
		if (!strcmp(name,".strtab")) {
			kernel_elf_symbols.strtab = (const char*)sh[i].sh_addr;
			kernel_elf_symbols.strtab_size = sh[i].sh_size;
		} else if (!strcmp(name,".symtab")) {
			kernel_elf_symbols.symtab = (elf_symbol_t*)sh[i].sh_addr;
			kernel_elf_symbols.symtab_size = sh[i].sh_size;
		}
	}
}

/*
 * Iterate through all the symbols and look for functions...
 * Then, as we find functions, check if the symbol is within that
 * function's range (given by value and size)
 */
const char* ELF_lookup_symbol_function(uint32_t addr, elf_symbols_t* elf) {
    int i;
    int num_symbols = elf->symtab_size / sizeof(elf_symbol_t);

    for (i = 0; i < num_symbols; i++) {
        if ((addr >= elf->symtab[i].value) && (addr <= (elf->symtab[i].value + elf->symtab[i].size))) {
            const char* name = (const char*)((uint32_t)elf->strtab + elf->symtab[i].name_offset_in_strtab);
            return name;
        }
    }
    
    return "NOT FOUND";
}

const char* ELF_lookup_function(uint32_t addr) {
    return ELF_lookup_symbol_function(addr, &kernel_elf_symbols);
}

ELF32_program* ELF_read(int ci, int type) {
    ELF32_program* program = ALC_malloc(sizeof(ELF32_program), type);
    CInfo_t info;
    current_vfs->objstat(ci, &info);
    
    if (info.type != STAT_FILE) {
        kprintf("\n[%s %i] Error: Not File.\n", __FILE__, __LINE__);
        return NULL;
    }

    //==========================
    // Load ELF header
    //==========================

        Elf32_Ehdr* header = ALC_malloc(sizeof(Elf32_Ehdr), type);
        current_vfs->read(ci, (uint8_t*)header, 0, sizeof(Elf32_Ehdr));
        if (header->e_ident[0] != '\x7f' || header->e_ident[1] != 'E') {
            kprintf("\n[%s %i] Error: Not ELF.\n", __FILE__, __LINE__);
            ALC_free(header, type);
            return NULL;
        }

        if (header->e_type != ET_EXEC && header->e_type != ET_DYN) {
            kprintf("\n[%s %i] Error: Program is not an executable or dynamic executable.\n", __FILE__, __LINE__);
            ALC_free(header, type);
            return NULL;
        }

    //==========================
    // Load ELF header
    //==========================
    // Load program header
    //==========================

        Elf32_Phdr* program_headers = ALC_malloc(sizeof(Elf32_Phdr) * header->e_phnum, type);
        current_vfs->read(ci, (uint8_t*)program_headers, header->e_phoff, sizeof(Elf32_Phdr) * header->e_phnum);

        program->entry_point = (void*)header->e_entry;
        uint32_t header_num  = header->e_phnum;

        ALC_free(header, type);

    //==========================
    // Load program header
    //==========================
    // Copy data to vELF location
    //==========================

        program->pages = ALC_malloc(header_num * sizeof(uint32_t), type);
        program->pages_count = header_num;
        for (uint32_t i = 0; i < header_num; i++) {
            if (program_headers[i].p_type != PT_LOAD) continue;

            uint32_t program_pages   = program_headers[i].p_memsz / PAGE_SIZE;
            uint32_t virtual_address = program_headers[i].p_vaddr;
            program->pages[i] = program_headers[i].p_vaddr;

            if (program_headers[i].p_memsz % PAGE_SIZE > 0) program_pages++;
            for (uint32_t i = 0; i < program_pages; i++) {
                ALC_mallocp(virtual_address, type);
                virtual_address += PAGE_SIZE;
            }

            memset((void*)program_headers[i].p_vaddr, 0, program_headers[i].p_memsz);
            current_vfs->read(ci, (uint8_t*)program_headers[i].p_vaddr, program_headers[i].p_offset, program_headers[i].p_memsz);
        }

    //==========================
    // Copy data to vELF location
    //==========================

    ALC_free(program_headers, type);
    return program;
}

void ELF_free_program(ELF32_program* program, uint8_t type) {
    for (uint32_t i = 0; i < program->pages_count; i++) 
        _kfreep((void*)program->pages[i]);

    ALC_free(program->pages, type);
    ALC_free(program, type);
}

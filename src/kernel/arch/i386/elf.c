// Thanks to: https://github.com/Jorl17/jOS/blob/master/elf.c#L61
//            https://github.com/makerimages/SwormOS/tree/master/kernel
#include <elf.h>

static elf_symbols_t _kernel_elf_symbols = { 0 };
static ELF32_program _elf_program = { 0 };
static Elf32_Ehdr _elf_header = { 0 };
static Elf32_Phdr _elf_program_headers[ELF_MAX_PROGRAM_HEADERS] = { 0 };

int ELF_build_symbols_from_multiboot(uint32_t header_addr, uint32_t header_shndx, uint32_t header_num) {
	Elf32_Shdr* sh = (Elf32_Shdr*)(header_addr);
	uint32_t shstrtab = sh[header_shndx].sh_addr;

	for (uint32_t i = 0; i < header_num; i++) {
		const char* name = (const char*) (shstrtab + sh[i].sh_name);
		if (!strcmp(name,".strtab")) {
			_kernel_elf_symbols.strtab = (const char*)sh[i].sh_addr;
			_kernel_elf_symbols.strtab_size = sh[i].sh_size;
		} 
        else if (!strcmp(name,".symtab")) {
			_kernel_elf_symbols.symtab = (elf_symbol_t*)sh[i].sh_addr;
			_kernel_elf_symbols.symtab_size = sh[i].sh_size;
		}
	}

    return 1;
}

/*
 * Iterate through all the symbols and look for functions...
 * Then, as we find functions, check if the symbol is within that
 * function's range (given by value and size)
 */
static const char* _lookup_symbol_function(uint32_t addr, elf_symbols_t* elf) {
    for (int i = 0; i < elf->symtab_size / sizeof(elf_symbol_t); i++) {
        if ((addr >= elf->symtab[i].value) && (addr <= (elf->symtab[i].value + elf->symtab[i].size))) {
            return (const char*)((uint32_t)elf->strtab + elf->symtab[i].name_offset_in_strtab);
        }
    }
    
    return "<UNDEFINED>";
}

const char* ELF_lookup_function(uint32_t addr) {
    return _lookup_symbol_function(addr, &_kernel_elf_symbols);
}

ELF32_program* ELF_read(int ci, int type) {
    ELF32_program* program = &_elf_program;
    memset(program, 0, sizeof(ELF32_program));

    CInfo_t info;
    current_vfs->objstat(ci, &info);
    if (info.type != STAT_FILE) {
        LOG("Error: Not a file!");
        return NULL;
    }

    Elf32_Ehdr* header = &_elf_header;
    memset(header, 0, sizeof(Elf32_Ehdr));

    current_vfs->read(ci, (uint8_t*)header, 0, sizeof(Elf32_Ehdr));
    if (header->e_ident[0] != '\x7f' || header->e_ident[1] != 'E') {
        LOG("Error: Not ELF executable!");
        return NULL;
    }

    if (header->e_type != ET_EXEC && header->e_type != ET_DYN) {
        LOG("Error: Program is not an executable or dynamic executable.");
        return NULL;
    }

    if (header->e_phnum > ELF_MAX_PROGRAM_HEADERS) {
        LOG("Error: Too many ELF program headers.");
        return NULL;
    }

    Elf32_Phdr* program_headers = _elf_program_headers;
    memset(program_headers, 0, sizeof(_elf_program_headers));

    current_vfs->read(ci, (uint8_t*)program_headers, header->e_phoff, sizeof(Elf32_Phdr) * header->e_phnum);
    program->entry_point = (void*)header->e_entry;
    uint32_t header_num  = header->e_phnum;

    for (uint32_t i = 0; i < header_num; i++) {
        if (program_headers[i].p_type != PT_LOAD) continue;

        uint32_t virtual_address = program_headers[i].p_vaddr & ~(PAGE_SIZE - 1);
        uint32_t segment_offset  = program_headers[i].p_vaddr - virtual_address;
        uint32_t program_pages   = (segment_offset + program_headers[i].p_memsz) / PAGE_SIZE;

        if ((segment_offset + program_headers[i].p_memsz) % PAGE_SIZE > 0) program_pages++;
        for (uint32_t page = 0; page < program_pages; page++) {
            if (program->pages_count >= ELF_MAX_PROGRAM_PAGES) {
                LOG("Error: ELF program uses too many pages.");
                return NULL;
            }

            program->pages[program->pages_count++] = virtual_address;
            ALC_mallocp(virtual_address, type);
            virtual_address += PAGE_SIZE;
        }

        memset((void*)program_headers[i].p_vaddr, 0, program_headers[i].p_memsz);
        current_vfs->read(ci, (uint8_t*)program_headers[i].p_vaddr, program_headers[i].p_offset, program_headers[i].p_filesz);
    }

    return program;
}

int ELF_free_program(ELF32_program* program, uint8_t type) {
    for (uint32_t i = 0; i < program->pages_count; i++) {
        ALC_freep((void*)program->pages[i], type);
    }

    memset(program, 0, sizeof(ELF32_program));
    return 1;
}

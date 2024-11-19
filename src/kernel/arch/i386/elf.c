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

ELF32_program* ELF_read(const char* path, int type) {
    ELF32_program* program = malloc(sizeof(ELF32_program));
    Content* content = current_vfs->getobj(path);
    if (content->file == NULL) {
        kprintf("[%s %i] File not found\n", __FILE__, __LINE__);
        FSLIB_unload_content_system(content);
        return NULL;
    }

    //==========================
    // Load ELF header
    //==========================

        void* header = malloc(sizeof(Elf32_Ehdr));
        current_vfs->readoff(content, header, 0, sizeof(Elf32_Ehdr));
        Elf32_Ehdr* ehdr = (Elf32_Ehdr*)header;
        if (ehdr->e_ident[0] != '\x7f' || ehdr->e_ident[1] != 'E') {
            kprintf("\n[%s %i] Error: Not ELF.\n", __FILE__, __LINE__);
            free(header);
            FSLIB_unload_content_system(content);
            return NULL;
        }

        if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) {
            kprintf("\n[%s %i] Error: Program is not an executable or dynamic executable.\n", __FILE__, __LINE__);
            free(header);
            FSLIB_unload_content_system(content);
            return NULL;
        }

    //==========================
    // Load ELF header
    //==========================
    // Load program header
    //==========================

        void* program_header = malloc(sizeof(Elf32_Phdr) * ehdr->e_phnum);
        current_vfs->readoff(content, program_header, ehdr->e_phoff, sizeof(Elf32_Phdr) * ehdr->e_phnum);
        Elf32_Phdr* phdr = (Elf32_Phdr*)program_header;

        program->entry_point = (void*)ehdr->e_entry;
        uint32_t header_num  = ehdr->e_phnum;

        free(header);

    //==========================
    // Load program header
    //==========================
    // Copy data to vELF location
    //==========================

        program->pages = malloc(header_num * sizeof(uint32_t));
        program->pages_count = header_num;
        for (uint32_t i = 0; i < header_num; i++) {
            if (phdr[i].p_type != PT_LOAD) continue;

            uint32_t program_pages   = phdr[i].p_memsz / PAGE_SIZE;
            uint32_t virtual_address = phdr[i].p_vaddr;
            program->pages[i] = phdr[i].p_vaddr;

            if (phdr[i].p_memsz % PAGE_SIZE > 0) program_pages++;
            for (uint32_t i = 0; i < program_pages; i++) {
                mallocp(virtual_address);
                virtual_address += PAGE_SIZE;
            }

            memset((void*)phdr[i].p_vaddr, 0, phdr[i].p_memsz);
            current_vfs->readoff(content, (uint8_t*)phdr[i].p_vaddr, phdr[i].p_offset, phdr[i].p_memsz);
        }

    //==========================
    // Copy data to vELF location
    //==========================

    free(program_header);
    FSLIB_unload_content_system(content);
    return program;
}

void ELF_free_program(ELF32_program* program) {
    for (uint32_t i = 0; i < program->pages_count; i++) 
        freep((void*)program->pages[i]);

    free(program->pages);
    free(program);
}

#include "multiboot.h"


struct Elf32_sectionHeader* MB2_get_elf_section(multiboot_info_t* info, char* section) {
    multiboot_elf_section_header_table_t section_table = info->u.elf_sec;

    uint32_t addr = section_table.addr;
    uint32_t num_sections = section_table.num;
    uint32_t shndx = section_table.shndx;

    clrscr();
    kprintf("%i\n", shndx);

    // struct Elf32_sectionHeader* section_header_table = (struct Elf32_sectionHeader*)addr;
    // uint32_t string_table_start = section_header_table[shndx].sh_addr;

    // for (uint32_t i = 0; i < num_sections; i++) {
    //     uint32_t sh_name = section_header_table[i].sh_name;
    //     if (sh_name != 0) {
    //         char* name = (char*)(string_table_start + sh_name);

    //         int diff = strcmp(name, section);
    //         if (diff == 0) return section_header_table + i;
    //     }
    // }

    return NULL;
}
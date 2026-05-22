#ifndef ELF_EXECUTE_H_
#define ELF_EXECUTE_H_

#include <stdint.h>
#include "stdlib.h"

#define ELF_MAX_PROGRAM_PAGES 512

struct ELF_program {
    uint32_t pages[ELF_MAX_PROGRAM_PAGES];
    uint32_t pages_count;
    void*    entry_point;
};

struct ELF_program* get_entry_point(char* path);
int execute(struct ELF_program* program, int argc, char* argv[]);
void free_program(struct ELF_program* program);

#endif

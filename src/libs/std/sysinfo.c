#include "../include/sysinfo.h"


void print_malloc_map() {
    __asm__ volatile (
        "movl $48, %%eax\n"
        "int $0x80\n"
        :
        :
        : "eax"
    );
}

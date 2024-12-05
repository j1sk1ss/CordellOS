#include "../include/assert.h"


void assert_failed(char* file, int line) {
    char message[512] = { 0 };
    sprintf(message, 512, 
        "ASSERT FAILED\nPROCCESS: %i\nFILE: %s\nLINE: %i",
        tpid(), file, line);

    panic(message);
}

void panic(char* message) {
    __asm__ volatile(
        "movl $51, %%eax\n"
        "movl %0, %%ecx\n"
        "int $0x80\n"
        : 
        : "r" (message)
        : "%eax", "%ecx"
    );
}
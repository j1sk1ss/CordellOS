#include "../include/keyboard.h"

static int _flush_keyboard() {
    __asm__ volatile (
        "movl $46, %%eax\n"
        "int $0x80\n"
        :
        : 
        : "eax"
    );

    return 1;
}

char get_char() {
    char key = 0;
    __asm__ volatile(
        "movl $5, %%eax\n"
        "movl %0, %%ecx\n"
        "int $0x80\n"
        :
        : "r"(&key)
        : "eax", "ecx"
    );

    return key;
}

char wait_char() {
    _flush_keyboard();
    char input = '\15';
    while (input == '\15') { input = get_char(); }
    return input;
}

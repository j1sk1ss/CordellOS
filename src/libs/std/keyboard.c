#include "../include/keyboard.h"


//====================================================================
//  This function clear last character of keyboard to \15
void _flush_keyboard() {
    __asm__ volatile (
        "movl $46, %%eax\n"
        "int $0x80\n"
        :
        : 
        : "eax"
    );
}

//====================================================================
// Function take a value from keyboard
// ECX - pointer to character
char get_char() {
    char key = 0;
    __asm__ volatile(
        "movl $5, %%eax\n"
        "movl %0, %%ecx\n"
        "int %1\n"
        :
        : "r"(&key), "i"(SYSCALL_INTERRUPT)
        : "eax", "ecx"
    );

    return key;
}

// NULL for stop list for using default \n stop char
void keyboard_read(char* stop_list, char* buffer) {
    _flush_keyboard(stop_list, buffer);
    while (stop_list[0] != '\250') { continue; }
}

// Wait keyboard interaction
char keyboard_wait() {
    char buffer[1] = { '\0' };
    while (buffer[0] == '\0') { wait_char(buffer); continue; }
    return buffer[0];
}

char wait_char() {
    _flush_keyboard();
    char input = '\15';
    while (input == '\15') { input = get_char(); }
    return input;
}
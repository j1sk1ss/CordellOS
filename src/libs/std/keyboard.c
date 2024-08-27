#include "../include/keyboard.h"

//   _  _________   __  ____   ___    _    ____  ____  
//  | |/ / ____\ \ / / | __ ) / _ \  / \  |  _ \|  _ \ 
//  | ' /|  _|  \ V /  |  _ \| | | |/ _ \ | |_) | | | |
//  | . \| |___  | |   | |_) | |_| / ___ \|  _ <| |_| |
//  |_|\_\_____| |_|   |____/ \___/_/   \_\_| \_\____/


// NULL for stop list for using default \n stop char
void keyboard_read(int mode, uint8_t color, char* stop_list, char* buffer) {
    input_read_stop(mode, color, stop_list, buffer);
    while (stop_list[0] != '\250') { continue; }
}

// Wait keyboard interaction
char keyboard_wait() {
    char buffer[1] = { '\0' };
    wait_char(buffer);

    while (buffer[0] == '\0') { continue; }
    return buffer[0];
}

//====================================================================
//  This function reads keyboard input from user until he press ENTER -
//  that returns string of input
//  EAX - interrupt / buffer
//  EDX - color
//  EBX - mode
void input_read(int mode, uint8_t color, char* buffer) {
    __asm__ volatile(
        "mov $19, %%rax\n"
        "mov %1, %%rbx\n"
        "mov %0, %%rdx\n"
        "mov %2, %%rcx\n"
        "int $0x80\n"
        : 
        : "r"((uint64_t)color), "r"((uint64_t)mode), "m"(buffer)
        : "rax", "rbx", "rcx", "rdx"
    );
}

//====================================================================
//  This function reads keyboard input from user until he press stop character -
//  that returns string of input with stop character of last position
//  EAX - interrupt / buffer
//  EDX - color
//  EBX - mode
//  ECX - stop list
//  ESI - buffer pointer
void input_read_stop(int mode, uint8_t color, char* stop_list, char* buffer) {
    __asm__ volatile (
        "mov $46, %%rax\n"
        "mov %1, %%rbx\n"
        "mov %0, %%rdx\n"
        "mov %2, %%rcx\n"
        "mov %3, %%rsi\n"
        "int $0x80\n"
        :
        : "m"(color), "m"(mode), "m"(stop_list), "m"(buffer)
        : "rax", "rbx", "rdx", "rsi"
    );
}

//====================================================================
// Function take a value from keyboard
// ECX - pointer to character
char get_char() {
    char key;
    __asm__ volatile(
        "mov $5, %%rax\n"
        "mov %0, %%rcx\n"
        "int %1\n"
        :
        : "r"(&key), "i"(SYSCALL_INTERRUPT)
        : "rax", "rbx", "rcx", "rdx"
    );

    return key;
}

//====================================================================
//  This function waits an any button pressing from user
//  ECX - pointer to character
char wait_char() {
    char buffer[10];
    char stop[2] = { STOP_KEYBOARD, '\0' };

    input_read_stop(HIDDEN_KEYBOARD, -1, stop, buffer);
    while (stop[0] != '\250') { continue; }
    
    return buffer[0];
}
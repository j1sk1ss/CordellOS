#include <networking.h>

//====================================================================
// Function set IP of host machine
// EBX - IP pointer
void set_ip(uint8_t* ip) {
    __asm__ volatile(
        "movl $41, %%eax\n"
        "movl %0, %%ebx\n"
        "int $0x80\n"
        :
        : "r" (ip)
        : "%eax", "%ebx"
    );
}

//====================================================================
// Function get IP of host machine
// EBX - IP buffer pointer
void get_ip(uint8_t* buffer) {
    __asm__ volatile(
        "movl $42, %%eax\n"
        "movl %0, %%ebx\n"
        "int $0x80\n"
        :
        : "r"(buffer)
        : "eax", "ebx", "ecx", "edx"
    );
}

//====================================================================
// Function get MAC of Ethernet card
// EBX - MAC buffer pointer
void get_mac(uint8_t* buffer) {
    __asm__ volatile(
        "movl $43, %%eax\n"
        "movl %0, %%ebx\n"
        "int $0x80\n"
        :
        : "r"(buffer)
        : "eax", "ebx", "ecx", "edx"
    );
}

//====================================================================
// Function send UDP package by Ethernet card
// EBX - DST IP address
// ECX - SRC port from data coming
// EDX - DST port
// ESI - Data pointer
// EDI - Data length
void send_udp_packet(uint8_t* dst_ip, uint16_t src_port, uint16_t dst_port, void* data, int len) {
    __asm__ volatile(
        "movl $38, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "movl %2, %%edx\n"
        "movl %3, %%esi\n"
        "movl %4, %%edi\n"
        "int $0x80\n"
        :
        : "g"(dst_ip), "g"(src_port), "g"(dst_port), "g"(data), "g"(len)
        : "eax", "ebx", "ecx", "edx", "esi", "edi"
    );
}

//====================================================================
// Function get pixel from framebuffer by coordinates
// EBX - data pointer
void pop_received_udp_packet(uint8_t* buffer) {
    __asm__ volatile(
        "movl $39, %%eax\n"
        "movl %0, %%ebx\n"
        "int $0x80\n"
        :
        : "r"(buffer)
        : "eax", "ebx", "ecx", "edx"
    );
}
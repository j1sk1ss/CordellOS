#include "../include/networking.h"


//====================================================================
// Function set IP of host machine
// EBX - IP pointer
void set_ip(uint8_t* ip) {
    __asm__ volatile(
        "mov $41, %%rax\n"
        "mov %0, %%rbx\n"
        "int $0x80\n"
        :
        : "r" (ip)
        : "%rax", "%rbx"
    );
}

//====================================================================
// Function get IP of host machine
// EBX - IP buffer pointer
void get_ip(uint8_t* buffer) {
    __asm__ volatile(
        "mov $42, %%rax\n"
        "mov %0, %%rbx\n"
        "int $0x80\n"
        :
        : "r"(buffer)
        : "rax", "rbx", "ecx", "edx"
    );
}

//====================================================================
// Function get MAC of Ethernet card
// EBX - MAC buffer pointer
void get_mac(uint8_t* buffer) {
    __asm__ volatile(
        "mov $43, %%rax\n"
        "mov %0, %%rbx\n"
        "int $0x80\n"
        :
        : "r"(buffer)
        : "rax", "rbx", "ecx", "edx"
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
        "mov $38, %%rax\n"
        "mov %0, %%rbx\n"
        "mov %1, %%ecx\n"
        "mov %2, %%edx\n"
        "mov %3, %%esi\n"
        "mov %4, %%edi\n"
        "int $0x80\n"
        :
        : "g"(dst_ip), "g"(src_port), "g"(dst_port), "g"(data), "g"(len)
        : "rax", "rbx", "ecx", "edx", "esi", "edi"
    );
}

//====================================================================
// Function get pixel from framebuffer by coordinates
// EBX - data pointer
void pop_received_udp_packet(uint8_t* buffer) {
    __asm__ volatile(
        "mov $39, %%rax\n"
        "mov %0, %%rbx\n"
        "int $0x80\n"
        :
        : "r"(buffer)
        : "rax", "rbx", "ecx", "edx"
    );
}
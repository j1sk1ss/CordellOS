#include <x86.h>

void i386_reboot() {
    i386_outb(0x64, 0xFE);
}

uint32_t i386_inl(uint16_t port) {
    uint32_t rv;
    asm("inl %%dx, %%eax" : "=a" (rv) : "dN" (port));
    return rv;
}

void i386_outl(uint16_t port, uint32_t data) {
    asm("outl %%eax, %%dx" : : "dN" (port), "a" (data));
}

uint16_t i386_inw(uint16_t port) {
    uint16_t r;
    asm("inw %1, %0" : "=a" (r) : "dN" (port));
    return r;
}

void i386_outw(uint16_t port, uint16_t data) {
    asm("outw %1, %0" : : "dN" (port), "a" (data));
}

uint8_t i386_inb(uint16_t port) {
    uint8_t r;
    asm("inb %1, %0" : "=a" (r) : "dN" (port));
    return r;
}

void i386_outb(uint16_t port, uint8_t data) {
    asm("outb %1, %0" : : "dN" (port), "a" (data));
}

uint8_t mem_inb(uint32_t addr) {
    return *((uint8_t*)(addr));
}

uint16_t mem_inw (uint32_t addr) {
    return *((uint16_t*)(addr));
}

uint32_t mem_inl(uint32_t addr) {
    return *((uint32_t*)(addr));
}

void mem_outb(uint32_t addr, uint8_t value) {
    (*((uint8_t*)(addr))) = (value);
}

void mem_outw(uint32_t addr, uint16_t value) {
    (*((uint16_t*)(addr))) = (value);
}

void mem_outl(uint32_t addr, uint32_t value) {
    (*((uint32_t*)(addr))) = (value);
}

void i386_io_wait() {
    i386_outb(UNUSED_PORT, 0);
}
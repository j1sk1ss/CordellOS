#include <arch/i386/x86.h>

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

void i386_io_wait() {
    i386_outb(UNUSED_PORT, 0);
}
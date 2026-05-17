#ifndef X86_H_
#define X86_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define ASMCALL     __attribute__((cdecl))
#define asm         __asm__ volatile
#define UNUSED_PORT 0x80

uint32_t i386_inl(uint16_t port);
void i386_outl(uint16_t port, uint32_t data);
uint16_t i386_inw(uint16_t port);
void i386_outw(uint16_t port, uint16_t data);
uint8_t i386_inb(uint16_t port);
void i386_outb(uint16_t port, uint8_t data);

uint8_t __attribute__((cdecl)) i386_enable_interrupts();
uint8_t __attribute__((cdecl)) i386_disable_interrupts();

void __attribute__((cdecl)) i386_switch2user(void* entry, void* stack);
void __attribute__((cdecl)) i386_panic();
char __attribute__((cdecl)) i386_inputWait();

void i386_io_wait();
void i386_reboot();

#define kernel_panic(data) kprintf("\n%s\n", data); i386_panic();

#endif

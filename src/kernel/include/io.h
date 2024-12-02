#ifndef IO_H_
#define IO_H_

#include <stdint.h>


#define ASMCALL __attribute__((cdecl))
#define asm __asm__ volatile


void __attribute__((cdecl)) i386_outb(uint16_t port, uint8_t data);
uint8_t __attribute__((cdecl)) i386_inb(uint16_t port);
uint8_t __attribute__((cdecl)) i386_enableInterrupts();
uint8_t __attribute__((cdecl)) i386_disableInterrupts();

char __attribute__((cdecl)) i386_inputWait();

void i386_io_wait();

#endif
#ifndef IO_H_
#define IO_H_


#include <stdint.h>


#define ASMCALL 
#define asm __asm__ volatile


void  i386_outb(uint16_t port, uint8_t data);
uint8_t  i386_inb(uint16_t port);
uint8_t  i386_enableInterrupts();
uint8_t  i386_disableInterrupts();

char  i386_inputWait();

void i386_io_wait();

#endif
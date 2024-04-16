#ifndef ISR_H
#define ISR_H


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "pit.h"
#include "idt.h"
#include "gdt.h"
#include "x86.h"
#include "stdio.h"
#include "elf.h"

#include "../util/arrays.h"


struct Registers {
    uint32_t ds;                                            // data segment pushed by us
    uint32_t edi, esi, ebp, kern_esp, ebx, edx, ecx, eax;   // pusha
    uint32_t interrupt, error;                              // we push interrupt and error code
    uint32_t eip, cs, eflag, esp, ss;                       // pushed auto by cpu
} __attribute__((packed));

struct stackframe { 
  struct stackframe* ebp; 
  uint32_t eip; 
}; 

typedef void (*ISRHandler)(struct Registers* regs);


void i386_isr_initialize();
void i386_isr_registerHandler(int interrupt, ISRHandler handler);

void i386_isr_interrupt_details(uint32_t eip, uint32_t ebp, uint32_t esp);
void i386_isr_stack_trace_line(uint32_t eip);

struct ELF32_symbols_desctiptor;
void i386_isr_set_symdes(struct ELF32_symbols_desctiptor* desciptor);

#endif

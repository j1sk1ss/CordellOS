#ifndef IRQ_H
#define IRQ_H


#include <stddef.h>

#include "x86.h"
#include "isr.h"
#include "stdio.h"
#include "pic.h"
#include "i8259.h"

#include "../util/arrays.h"


struct Registers;
typedef void (*IRQHandler)(struct Registers* regs);


void i386_irq_initialize();
void i386_irq_registerHandler(int irq, IRQHandler handler);

#endif
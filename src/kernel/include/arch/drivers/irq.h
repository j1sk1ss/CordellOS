#ifndef IRQ_H
#define IRQ_H

#include <stddef.h>

#include <arch/i386/x86.h>
#include <arch/drivers/isr.h>
#include <arch/drivers/pic.h>
#include <arch/drivers/i8259.h>
#include <graphics/kstdio.h>
#include <util/arrays.h>

#define PIC_REMAP_OFFSET 0x20

struct Registers;
typedef void (*IRQHandler)(struct Registers* regs);
int i386_irq_initialize();
void i386_irq_registerHandler(int irq, IRQHandler handler);

#endif

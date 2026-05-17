#ifndef HAL_H_
#define HAL_H_

#include <arch/drivers/isr.h>
#include <arch/i386/gdt.h>
#include <arch/drivers/idt.h>
#include <arch/drivers/irq.h>
#include <stdio.h>
#include <arch/i386/tss.h>

void HAL_initialize();

#endif
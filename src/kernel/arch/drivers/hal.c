#include "../../include/hal.h"


void HAL_initialize() {
    i386_gdt_initialize();
    kprintf("GDT INIZIALIZZATO\n");

    i386_idt_initialize();
    kprintf("IDT INIZIALIZZATO\n");

    i386_isr_initialize();
    kprintf("ISR INIZIALIZZATO\n");

    i386_irq_initialize();
    kprintf("IRQ INIZIALIZZATO\n");

    TSS_init(0x5, 0x10, 0x0);
    kprintf("TSS INIZIALIZZATO\n");
}

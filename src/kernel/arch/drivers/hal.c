#include "../../include/hal.h"


void HAL_initialize() {
    kprintf("HAL: (");
    i386_gdt_initialize();
    kprintf("GDT\t");

    i386_idt_initialize();
    kprintf("IDT\t");

    i386_isr_initialize();
    kprintf("ISR\t");

    if (i386_irq_initialize()) 
        kprintf("IRQ\t");

    TSS_init(0x5, 0x10, 0x0);
    kprintf("TSS)\n");
}

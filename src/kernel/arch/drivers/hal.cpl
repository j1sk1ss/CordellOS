extern function kprintf(ptr i8 fmt, ...) -> i0;
extern function i386_gdt_initialize() -> i0;
extern function i386_idt_initialize() -> i0;
extern function i386_isr_initialize() -> i0;
extern function i386_irq_initialize() -> i32;
extern function TSS_init(u32 idx, u32 kss, u32 kesp) -> i0;

glob function HAL_initialize() -> i0 {
    kprintf(ref "HAL: (");
    i386_gdt_initialize();
    kprintf(ref "GDT\t");

    i386_idt_initialize();
    kprintf(ref "IDT\t");

    i386_isr_initialize();
    kprintf(ref "ISR\t");

    if i386_irq_initialize(); {
        kprintf(ref "IRQ\t");
    }

    TSS_init(0x5 as u32, 0x10 as u32, 0x0 as u32);
    kprintf(ref "TSS)\n");
}
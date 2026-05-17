#include <arch/drivers/irq.h>

static IRQHandler _handler[16] = { NULL };
static const PICDriver* _pic_driver = NULL; 

void i386_irq_handler(struct Registers* regs) {
    int irq = regs->interrupt - PIC_REMAP_OFFSET;
    if (irq < 0 || irq >= 16) return;

    uint8_t pic_isr = (uint8_t)(uintptr_t)i8259_read_IRQ_in_service_registers();
    uint8_t pic_irr = (uint8_t)(uintptr_t)i8259_read_IRQ_request_registers();
    
    if (_handler[irq] != NULL) _handler[irq](regs);
    else kprintf("[%s %i] NO HANDLER FOR: %i | %i %i\n", __FILE__, __LINE__, irq, pic_isr, pic_irr);

    _pic_driver->SendEndOfInterrupt(irq);
}

int i386_irq_initialize() {
    const PICDriver* drivers[] = { i8259_get_driver(), };

    for (int i = 0; i < SIZE(drivers); i++) 
        if (drivers[i]->Probe()) _pic_driver = drivers[i];

    if (_pic_driver == NULL) {
        kprintf("[%s %i] WARN: NO PIC!\n", __FILE__, __LINE__);
        return 0;
    }
    
    kprintf("PIC %s FOUND!\n", _pic_driver->Name);
    _pic_driver->Initialize(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8, false);

    for (int i = 0; i < 16; i++) i386_isr_register_handler(PIC_REMAP_OFFSET + i, i386_irq_handler);
    i386_enable_interrupts();
    _pic_driver->Unmask(2); // slave interrupt controller allowing for IRQ 8-15
    return 1;
}

void i386_irq_registerHandler(int irq, IRQHandler handler) {
    if (irq < 0 || irq >= 16 || _pic_driver == NULL) return;
    _handler[irq] = handler;
    _pic_driver->Unmask(irq);
}

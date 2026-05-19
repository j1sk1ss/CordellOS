#include <arch/drivers/i8259.h>

static uint16_t picMask = 0xFFFF;

static void _set_mask(uint16_t newMask) {
    picMask = newMask;
    i386_outb(PIC1_DATA_PORT, picMask & 0xFF);                              // Lower 8 bits to PIC1                                       
    i386_io_wait();
    i386_outb(PIC2_DATA_PORT, picMask >> 8);                                // Upper 8 bits to PIC2
    i386_io_wait();
}

static inline uint16_t _get_mask() {
    return i386_inb(PIC1_DATA_PORT) | (i386_inb(PIC2_DATA_PORT) << 8);
}

static void _configure(uint8_t offsetPic1, uint8_t offsetPic2, bool autoEoi) {
    // Mask everything
    _set_mask(0xFFFF);

    // initialization control word 1
    i386_outb(PIC1_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);  // Send to PIC1 port init command
    i386_io_wait();                                                     // Wait for PC respond
    i386_outb(PIC2_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);  // Send to PIC2 port init command
    i386_io_wait();                                                     // Wait for PC respond

    // initialization control word 2 - the offsets
    i386_outb(PIC1_DATA_PORT, offsetPic1);
    i386_io_wait();
    i386_outb(PIC2_DATA_PORT, offsetPic2);
    i386_io_wait();

    // initialization control word 3
    i386_outb(PIC1_DATA_PORT, 0x4); // tell PIC1 that it has a slave at IRQ2 (0000 0100)
    i386_io_wait();
    i386_outb(PIC2_DATA_PORT, 0x2); // tell PIC2 its cascade identity (0000 0010)
    i386_io_wait();

    // initialization control word 4
    uint8_t icw4 = PIC_ICW4_8086;
    if (autoEoi) 
        icw4 |= PIC_ICW4_AUTO_EOI;

    i386_outb(PIC1_DATA_PORT, icw4);
    i386_io_wait();
    i386_outb(PIC2_DATA_PORT, icw4);
    i386_io_wait();

    // clear data registers
    _set_mask(0xFFFF);
}

static inline void _send_end_of_interrupt(int irq) {
    if (irq >= 8) i386_outb(PIC2_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
    i386_outb(PIC1_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
}

static inline void _disable() {
    _set_mask(0xFFFF);
}

// irq = interrupt request number
static inline void _mask(int irq) {                                               
    _set_mask(picMask | (1 << irq));
}

// irq = interrupt request number
static inline void _unmask(int irq) {                                             
    _set_mask(picMask & ~(1 << irq));
}

uint16_t i8259_read_IRQ_request_registers() {
    i386_outb(PIC1_COMMAND_PORT, PIC_CMD_READ_IRR);
    i386_outb(PIC2_COMMAND_PORT, PIC_CMD_READ_IRR);
    return (i386_inb(PIC2_DATA_PORT) | (i386_inb(PIC2_DATA_PORT) << 8));
}

uint16_t i8259_read_IRQ_in_service_registers() {
    i386_outb(PIC1_COMMAND_PORT, PIC_CMD_READ_ISR);
    i386_outb(PIC2_COMMAND_PORT, PIC_CMD_READ_ISR);
    return (i386_inb(PIC2_DATA_PORT) | (i386_inb(PIC2_DATA_PORT) << 8));
}

bool i8259_probe() {
    _disable();
    _set_mask(0x1488);
    return _get_mask() == 0x1488;
}

static const PICDriver _PICDriver = {
    .Name                   = "8259 PIC",
    .Probe                  = &i8259_probe,
    .Initialize             = &_configure,
    .Disable                = &i386_disable_interrupts,
    .SendEndOfInterrupt     = &_send_end_of_interrupt,
    .Mask                   = &_mask,
    .Unmask                 = &_unmask
};

const PICDriver* i8259_get_driver() {
    return &_PICDriver;
}
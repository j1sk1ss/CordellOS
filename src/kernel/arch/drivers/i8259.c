#include "../../include/i8259.h"


static uint16_t picMask = 0xFFFF;


void i8259_setMask(uint16_t newMask) {
    picMask = newMask;

    i386_outb(PIC1_DATA_PORT, picMask & 0xFF);                              // Lower 8 bits to PIC1                                       
    i386_io_wait();
    i386_outb(PIC2_DATA_PORT, picMask >> 8);                                // Upper 8 bits to PIC2
    i386_io_wait();
}

uint16_t i8259_getMask() {
    return i386_inb(PIC1_DATA_PORT) | (i386_inb(PIC2_DATA_PORT) << 8);
}

void i8259_configure(uint8_t offsetPic1, uint8_t offsetPic2, bool autoEoi) {
    // Mask everything
    i8259_setMask(0xFFFF);

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
    i8259_setMask(0xFFFF);
}

void i8259_sendEndOfInterrupt(int irq) {
     if (irq >= 8) i386_outb(PIC2_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
    i386_outb(PIC1_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
}

void i8259_disable() {
    i8259_setMask(0xFFFF);
}

// irq = interrupt request number
void i8259_mask(int irq) {                                               
    i8259_setMask(picMask | (1 << irq));
}

// irq = interrupt request number
void i8259_unmask(int irq) {                                             
     i8259_setMask(picMask & ~(1 << irq));
}

uint16_t i8259_readIRQRequestRegisters() {
    i386_outb(PIC1_COMMAND_PORT, PIC_CMD_READ_IRR);
    i386_outb(PIC2_COMMAND_PORT, PIC_CMD_READ_IRR);

    return (i386_inb(PIC2_DATA_PORT) | (i386_inb(PIC2_DATA_PORT) << 8));
}

uint16_t i8259_readIRQInServiceRegisters() {
    i386_outb(PIC1_COMMAND_PORT, PIC_CMD_READ_ISR);
    i386_outb(PIC2_COMMAND_PORT, PIC_CMD_READ_ISR);
    return (i386_inb(PIC2_DATA_PORT) | (i386_inb(PIC2_DATA_PORT) << 8));
}

bool i8259_probe() {
    i8259_disable();
    i8259_setMask(0x1488);
    return i8259_getMask() == 0x1488;
}


static const PICDriver _PICDriver = {
    .Name                   = "8259 PIC",
    .Probe                  = &i8259_probe,
    .Initialize             = &i8259_configure,
    .Disable                = &i386_disableInterrupts,
    .SendEndOfInterrupt     = &i8259_sendEndOfInterrupt,
    .Mask                   = &i8259_mask,
    .Unmask                 = &i8259_unmask
};


const PICDriver* i8259_getDriver() {
    return &_PICDriver;
}
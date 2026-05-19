#include <arch/drivers/idt.h>

static idt_entry_t _idt[256];
static idt_descriptor_t _idt_descriptor = { sizeof(_idt) - 1, _idt };

void i386_idt_setGate(int interrupt, void* base, uint16_t segmentDescriptor, uint8_t flags) {
    _idt[interrupt].base_low         = ((uint32_t)base) & 0xFFFF;
    _idt[interrupt].segment_selector = segmentDescriptor;
    _idt[interrupt].reserved         = 0;
    _idt[interrupt].flags            = flags;
    _idt[interrupt].base_high        = ((uint32_t)base >> 16) & 0xFFFF;
}

void i386_idt_enableGate(int interrupt) {
    FLAG_SET(_idt[interrupt].flags, IDT_FLAG_PRESENT);
}

void i386_idt_disableGate(int interrupt) {
    FLAG_UNSET(_idt[interrupt].flags, IDT_FLAG_PRESENT);
}

void i386_idt_initialize() {
    i386_idt_load(&_idt_descriptor);
}
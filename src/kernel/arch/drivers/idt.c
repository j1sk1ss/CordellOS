#include "../../include/idt.h"


static IDTEntry _idt[256];
static IDTDescriptor _IDTDescriptor = { sizeof(_idt) - 1, _idt };


void i386_idt_setGate(int interrupt, void* base, uint16_t segmentDescriptor, uint8_t flags) {
    _idt[interrupt].BaseLow         = ((uint32_t)base) & 0xFFFF;
    _idt[interrupt].SegmentSelector = segmentDescriptor;
    _idt[interrupt].Reserved        = 0;
    _idt[interrupt].Flags           = flags;
    _idt[interrupt].BaseHigh        = ((uint32_t)base >> 16) & 0xFFFF;
}

void i386_idt_enableGate(int interrupt) {
    FLAG_SET(_idt[interrupt].Flags, IDT_FLAG_PRESENT);
}

void i386_idt_disableGate(int interrupt) {
    FLAG_UNSET(_idt[interrupt].Flags, IDT_FLAG_PRESENT);
}

void i386_idt_initialize() {
    i386_idt_load(&_IDTDescriptor);
}
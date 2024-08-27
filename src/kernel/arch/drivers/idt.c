#include "../../include/idt.h"


typedef struct {
    uint16_t BaseLow;
    uint16_t SegmentSelector;
    uint8_t Reserved;
    uint8_t Flags;
    uint16_t BaseHigh;
} __attribute__((packed)) IDTEntry;

typedef struct {
    uint16_t Limit;
    IDTEntry* Ptr;
} __attribute__((packed)) IDTDescriptor;


IDTEntry _idt[256];
IDTDescriptor _IDTDescriptor = { sizeof(_idt) - 1, _idt };


void  i386_idt_load(IDTDescriptor* idtDescriptor);

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
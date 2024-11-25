#ifndef IDT_H_
#define IDT_H_


#include <stdint.h>

#include "../util/binary.h"


#define i386_GDT_CODE_SEGMENT 0x08
#define i386_GDT_DATA_SEGMENT 0x10


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

typedef enum {
    IDT_FLAG_GATE_TASK                      = 0x05,

    IDT_FLAG_GATE_16BIT_INT                 = 0x06,
    IDT_FLAG_GATE_16BIT_TRAP                = 0x07,

    IDT_FLAG_GATE_32BIT_INT                 = 0x0E,
    IDT_FLAG_GATE_32BIT_TRAP                = 0x0F,

    IDT_FLAG_RING0                          = (0 << 5),
    IDT_FLAG_RING1                          = (1 << 5),
    IDT_FLAG_RING2                          = (2 << 5),
    IDT_FLAG_RING3                          = (3 << 5),

    IDT_FLAG_PRESENT                        = 0x80,
} IDT_FLAGS;


void __attribute__((cdecl)) i386_idt_load(IDTDescriptor* idtDescriptor);

void i386_idt_initialize();
void i386_idt_disableGate(int interrupt);
void i386_idt_enableGate(int interrupt);

void i386_idt_setGate(int interrupt, void* base, uint16_t segmentDescriptor, uint8_t flags);

#endif
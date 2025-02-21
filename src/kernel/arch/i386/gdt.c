#include "../../include/gdt.h"


static GDTEntry _gtd[] = {
    // Empty discriptor
    GDT_ENTRY(0, 0, 0, 0),

    // Kernel 32-bit code segment
    GDT_ENTRY(0, 0xFFFFF,
        (GDT_ACCESS_SEGMENT_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE),
        (GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K)),

    // Kernel 32-bit data segment
    GDT_ENTRY(0, 0xFFFFF,
        (GDT_ACCESS_SEGMENT_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITABLE),
        (GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K)),

    // User 32-bit code segment
    GDT_ENTRY(0, 0xFFFFF,
        (GDT_ACCESS_SEGMENT_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE),
        (GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K)),

    // User 32-bit data segment
    GDT_ENTRY(0, 0xFFFFF,
        (GDT_ACCESS_SEGMENT_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITABLE),
        (GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K)),

    // TSS empty segment
    GDT_ENTRY(0, 0, 0, 0),
};

static GDTDescriptor _GDTDescriptor = { sizeof(_gtd) - 1, (GDTEntry*)&_gtd };


void __attribute__((cdecl)) i386_gdt_load(GDTDescriptor* descriptor, uint16_t codeSegment, uint16_t dataSegment);

void __attribute__((cdecl)) i386_gdt_initialize() {
    i386_gdt_load(&_GDTDescriptor, i386_GDT_CODE_SEGMENT, i386_GDT_DATA_SEGMENT);
}

void GDT_set_entry(int index, int base, int limit, uint8_t access, uint8_t flags) {
    _gtd[index].LimitLow     = limit & 0xFFFF;
    _gtd[index].BaseLow      = base & 0xFFFF;
    _gtd[index].BaseMiddle   = (base >> 16) & 0xFF;
    _gtd[index].Access       = access;
    _gtd[index].FlagsLimitHi = ((limit >> 16) & 0x0F) | (flags & 0xF0);
    _gtd[index].BaseHigh     = (base >> 24) & 0xFF;
}
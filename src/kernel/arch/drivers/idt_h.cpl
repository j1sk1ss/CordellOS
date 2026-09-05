#ifndef IDT_H_
#define IDT_H_ 0

#define i386_GDT_CODE_SEGMENT    0x08
#define i386_GDT_DATA_SEGMENT    0x10
#define IDT_FLAG_GATE_TASK       0x05
#define IDT_FLAG_GATE_16BIT_INT  0x06
#define IDT_FLAG_GATE_16BIT_TRAP 0x07
#define IDT_FLAG_GATE_32BIT_INT  0x0E
#define IDT_FLAG_GATE_32BIT_TRAP 0x0F
#define IDT_FLAG_RING0           (0 << 5)
#define IDT_FLAG_RING1           (1 << 5)
#define IDT_FLAG_RING2           (2 << 5)
#define IDT_FLAG_RING3           (3 << 5)
#define IDT_FLAG_PRESENT         0x80

@[align(1)]
container idt_entry {
    u16 base_low;
    u16 segm_sel;
    u8  reserved;
    u8  flags;
    u16 base_high;

    @[self]
    @[inline(model)]
    function set(ptr idt_entry self, ptr i0 base, u16 seg_des, u8 flags) -> i0;
}

@[align(1)]
container idt_descriptor {
    u16           limit;
    ptr idt_entry pointer;

    @[self]
    @[inline(model)]
    function init(ptr idt_descriptor self, ptr idt_entry entry, u16 size) -> i0;
}

glob function i386_idt_initialize() -> i0;
glob function i386_idt_setGate(i32 int, ptr i0 base, u16 seg_des, u8 flags) -> i0;

#endif

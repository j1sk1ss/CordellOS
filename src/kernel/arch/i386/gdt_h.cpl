#ifndef GDT_H_
#define GDT_H_ 0

#define i386_GDT_CODE_SEGMENT            0x08
#define i386_GDT_DATA_SEGMENT            0x10
#define i386_GDT_USER_CODE_SEGMENT       0x18
#define i386_GDT_USER_DATA_SEGMENT       0x20
#define GDT_ACCESS_CODE_READABLE         0x02
#define GDT_ACCESS_DATA_WRITABLE         0x02
#define GDT_ACCESS_CODE_CONFORMING       0x04
#define GDT_ACCESS_DATA_DIRECTION_NORMAL 0x00
#define GDT_ACCESS_DATA_DIRECTION_DOWN   0x04
#define GDT_ACCESS_DATA_SEGMENT          0x10
#define GDT_ACCESS_CODE_SEGMENT          0x18
#define GDT_ACCESS_DISCRIPTOR_TSS        0x89
#define GDT_ACCESS_RING0                 0x00
#define GDT_ACCESS_RING1                 0x20
#define GDT_ACCESS_RING2                 0x40
#define GDT_ACCESS_RING3                 0x60
#define GDT_ACCESS_SEGMENT_PRESENT       0x80
#define GDT_FLAG_64BIT                   0x20
#define GDT_FLAG_32BIT                   0x40
#define GDT_FLAG_16BIT                   0x00
#define GDT_FLAG_GRANULARITY_1B          0x00
#define GDT_FLAG_GRANULARITY_4K          0x80

@[inline(always)]
function _get_limit_low(u32 limit) -> u16 {
    (limit & 0xFFFF) as u16;
}

@[inline(always)]
function _get_base_low(u32 base) -> u16 {
    (base & 0xFFFF) as u16;
}

@[inline(always)]
function _get_base_middle(u32 base) -> u8 {
    ((base >> 16) & 0xFF) as u8;
}

@[inline(always)]
function _get_flags_limit_high(u32 limit, u8 flags) -> u8 {
    (((limit >> 16) & 0xF) | (flags & 0xF0)) as u8;
}

@[inline(always)]
function _get_base_high(u32 base) -> u8 {
    (((base >> 24) & 0xFF)) as u8;
}

glob function i386_gdt_initialize() -> i0;
glob function GDT_set_entry(i32 index, i32 base, i32 limit, u8 access, u8 flags) -> i0;

#endif

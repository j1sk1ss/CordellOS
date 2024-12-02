#ifndef GDT_H_
#define GDT_H_

#include <stdint.h>


#define i386_GDT_CODE_SEGMENT 0x08
#define i386_GDT_DATA_SEGMENT 0x10


void __attribute__((cdecl)) i386_gdt_initialize();
void GDT_set_entry(int index, int base, int limit, uint8_t access, uint8_t flags);

#endif
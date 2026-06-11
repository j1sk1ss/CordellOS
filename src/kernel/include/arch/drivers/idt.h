#ifndef IDT_H_
#define IDT_H_

#include <stdint.h>

extern void i386_idt_enableGate(int interrupt);
extern void i386_idt_setGate(int interrupt, void* base, uint16_t segmentDescriptor, uint8_t flags);

#endif

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define ASMCALL __attribute__((cdecl))
#define asm __asm__ volatile

uint16_t x86_inw(uint16_t port);
void x86_outw(uint16_t port, uint16_t data);
uint8_t x86_inb(uint16_t port);
void x86_outb(uint16_t port, uint8_t data);
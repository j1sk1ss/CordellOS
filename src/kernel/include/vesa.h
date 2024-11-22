#ifndef VESA_H_
#define VESA_H_

#include <memory.h>

#include "stdio.h"
#include "gfx.h"


#define CHAR_BODY	0x0000000F
#define EMPTY_SPACE	0x000000F0


void VESA_init();

void VESA_clrscr();
void VESA_fill(uint32_t color);
void VESA_newline();

void VESA_putchr(uint8_t x, uint8_t y, char c);
void VESA_cputchr(uint8_t x, uint8_t y, char c, uint32_t fcolor, uint32_t bcolor);
void VESA_putc(char c);
void VESA_cputc(char c, uint32_t fcolor, uint32_t bcolor);

void VESA_set_cursor(uint8_t x, uint8_t y);
void VESA_set_cursor32(uint32_t x, uint32_t y);

int VESA_get_cursor32_x();
uint8_t VESA_get_cursor_x();
int VESA_get_cursor32_y();
uint8_t VESA_get_cursor_y();
int VESA_get_max32_x();
uint8_t VESA_get_max_x();
int VESA_get_max32_y();
uint8_t VESA_get_max_y();

void __vmem_fill(uint32_t color, uint32_t a, uint32_t b, uint32_t c, uint32_t d);
void __vmem_putc(int x, int y, char c, uint32_t foreground, uint32_t background);
char __vmem_getc(int x, int y);

#endif
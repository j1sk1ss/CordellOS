#ifndef VESA_H_
#define VESA_H_


#include "gfx.h"
#include "keyboard.h"

#include <memory.h>


void VESA_clrscr();

void VESA_newline();

void VESA_putc(char c);
void VESA_cputc(char c, uint32_t fcolor, uint32_t bcolor);

void VESA_backspace();
void VESA_set_cursor(uint8_t x, uint8_t y);
void VESA_set_cursor32(uint32_t x, uint32_t y);

int VESA_get_cursor32_x();
int VESA_get_cursor_x();
int VESA_get_cursor32_y();
int VESA_get_cursor_y();
int VESA_get_max32_x();
int VESA_get_max_x();
int VESA_get_max32_y();
int VESA_get_max_y();

#endif
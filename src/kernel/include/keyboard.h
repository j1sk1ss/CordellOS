#ifndef KEYBOARD_H_
#define KEYBOARD_H_


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include "x86.h"
#include "irq.h"
#include "stdio.h"
#include "vga.h"
#include "vesa.h"


#define KBD_DATA_PORT           0x60

#define HIDDEN_KEYBOARD         0
#define VISIBLE_KEYBOARD        1

#define STOP_KEYBOARD           '\1'
#define DEL_BUTTON              '\2'
#define LSHIFT_BUTTON           '\3'
#define RSHIFT_BUTTON           '\4'
#define F4_BUTTON               '\5'
#define F3_BUTTON               '\6'
#define F2_BUTTON               '\7'
#define F1_BUTTON               '\10'

#define UP_ARROW_BUTTON         '\11'
#define DOWN_ARROW_BUTTON       '\12'
#define LEFT_ARROW_BUTTON       '\13'
#define RIGHT_ARROW_BUTTON      '\14'

#define ENTER_BUTTON            '\n'
#define BACKSPACE_BUTTON        '\b'

#define LSHIFT                  0x2A
#define RSHIFT                  0x36


struct Registers;
void i386_keyboard_handler(struct Registers* regs);
int key_press();
char get_character(char character);

void enable_keyboard(uint8_t* buffer, int keyboard_mode, int keyboard_color, uint8_t* stop);
char keyboard_navigation();
void keyboard_wait(char symbol);

void i386_init_keyboard();


#endif
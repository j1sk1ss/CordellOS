#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "kstdio.h"
#include "x86.h"
#include "irq.h"


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

#define EMPTY_KEYBOARD          '\15'

#define ENTER_BUTTON            '\n'
#define BACKSPACE_BUTTON        '\b'

#define LSHIFT                  0x2A
#define RSHIFT                  0x36


typedef struct keyboard_data {
    bool key_pressed[128];
} keyboard_data_t;


struct Registers;
void i386_keyboard_handler(struct Registers* regs);

void _enable_keyboard();
char pop_character();

void i386_init_keyboard();

#endif
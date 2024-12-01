#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_


#include <stdint.h>

#include "graphics.h"


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


void keyboard_read(char* stop_list, char* buffer);
void _flush_keyboard();
char get_char();

char keyboard_wait();
char wait_char();

#endif
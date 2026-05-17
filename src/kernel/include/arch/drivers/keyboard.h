#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <stddef.h>
#include <stdint.h>
#include <graphics/kstdio.h>
#include <arch/i386/x86.h>
#include <arch/drivers/irq.h>

#define KBD_DATA_PORT           0x60
#define KBD_STATUS_PORT         0x64
#define KBD_COMMAND_PORT        0x64

#define KBD_STATUS_OUTPUT_FULL  0x01
#define KBD_STATUS_INPUT_FULL   0x02

#define KBD_CMD_READ_CONFIG     0x20
#define KBD_CMD_WRITE_CONFIG    0x60
#define KBD_CMD_ENABLE_PORT1    0xAE

#define KBD_CONFIG_IRQ1         0x01
#define KBD_CONFIG_PORT1_CLOCK  0x10

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
    int key_pressed[128];
} keyboard_data_t;

struct Registers;
void i386_keyboard_handler(struct Registers* regs);

void enable_keyboard();
char pop_character();

void i386_init_keyboard();

#endif

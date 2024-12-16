#include "../../include/keyboard.h"

// Shift keyboard converter from https://github.com/cstack/osdev/blob/master/drivers/keyboard.c#L8

/* 
*  KBDUS means US keyboard Layout. This is a scancode table
*  used to layout a standard US keyboard. I have left some
*  comments in to give you an idea of what key is what, even
*  though I set it's array index to 0. You can change that to
*  whatever you want using a macro, if you wish! 
*/

static unsigned char _alphabet[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	                    /* 9 */
    '9', '0', '-', '=', '\b',	                                        /* Backspace */
    '\t',			                                                    /* Tab */
    'q', 'w', 'e', 'r',	                                                /* 19 */
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', ENTER_BUTTON,	            /* Enter key */
    0,			                                                        /* 29   - Control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	                /* 39 */
    '\'', '`', LSHIFT_BUTTON,		                                    /* Left shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n',			                        /* 49 */
    'm', ',', '.', '/',  RSHIFT_BUTTON,				                    /* Right shift */
    '*',
    0,	                                                                /* Alt */
    ' ',	                                                            /* Space bar */
    '\5',	                                                            /* Caps lock */
    F1_BUTTON,	                                                        /* 59 - F1 key ... > */
    F2_BUTTON,   F3_BUTTON,   F4_BUTTON,   0,   0,   0,   0,   0,                       
    0,	                                                                /* < ... F10 */
    0,	                                                                /* 69 - Num lock*/
    0,	                                                                /* Scroll Lock */
    0,	                                                                /* Home key */
    UP_ARROW_BUTTON,	                                                /* Up Arrow */
    0,	                                                                /* Page Up */
    '-',                        
    LEFT_ARROW_BUTTON,	                                                /* Left Arrow */
    0,                      
    RIGHT_ARROW_BUTTON,	                                                /* Right Arrow */
    '+',                        
    0,	                                                                /* 79 - End key*/
    DOWN_ARROW_BUTTON,	                                                /* Down Arrow */
    0,	                                                                /* Page Down */
    0,	                                                                /* Insert Key */
    DEL_BUTTON,	                                                        /* Delete Key */
    0,   0,   0,                        
    0,	                                                                /* F11 Key */
    0,	                                                                /* F12 Key */
    0,	                                                                /* All other keys are undefined */
};

static unsigned char _shift_alphabet[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*',	                    /* 9 */
    '(', ')', '_', '+', '\b',	                                        /* Backspace */
    '\t',			                                                    /* Tab */
    'Q', 'W', 'E', 'R',	                                                /* 19 */
    'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', ENTER_BUTTON,	            /* Enter key */
    0,			                                                        /* 29   - Control */
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',	                /* 39 */
    '"', '~', LSHIFT_BUTTON,		                                    /* Left shift */
    '|', 'Z', 'X', 'C', 'V', 'B', 'N',			                        /* 49 */
    'M', '<', '>', '?',  RSHIFT_BUTTON,				                    /* Right shift */
    '*',
    0,	                                                                /* Alt */
    ' ',	                                                            /* Space bar */
    '\5',	                                                            /* Caps lock */
    F1_BUTTON,	                                                        /* 59 - F1 key ... > */
    F2_BUTTON, F3_BUTTON, F4_BUTTON, 0, 0, 0, 0, 0,  
    0,	                                                                /* < ... F10 */
    0,	                                                                /* 69 - Num lock*/
    0,	                                                                /* Scroll Lock */
    0,	                                                                /* Home key */
    UP_ARROW_BUTTON,	                                                /* Up Arrow */
    0,	                                                                /* Page Up */
    '-',                        
    LEFT_ARROW_BUTTON,	                                                /* Left Arrow */
    0,                      
    RIGHT_ARROW_BUTTON,	                                                /* Right Arrow */
    '+',                        
    0,	                                                                /* 79 - End key*/
    DOWN_ARROW_BUTTON,	                                                /* Down Arrow */
    0,	                                                                /* Page Down */
    0,	                                                                /* Insert Key */
    DEL_BUTTON,	                                                        /* Delete Key */
    0,   0,   0,                        
    0,	                                                                /* F11 Key */
    0,	                                                                /* F12 Key */
    0,	                                                                /* All other keys are undefined */
};

static char _curr_char = EMPTY_KEYBOARD;
static keyboard_data_t _keyboard_data = {
    .key_pressed = { false }
};


void i386_init_keyboard() {
    i386_outb(0x64, 0xFF);
    uint8_t status = i386_inb(0x64);
    status = i386_inb(0x64);

    kprintf("[KEYBOARD INFO]: ( ");
    if (status & (1 << 0)) kprintf("Output buffer full.\t");
    else kprintf("Output buffer empty.\t");
    if (status & (1 << 1)) kprintf("Input buffer full.\t");
    else kprintf("Input buffer empty.\t");
    if (status & (1 << 2)) kprintf("System flag set.\t");
    else kprintf("System flag unset.\t");
    if (status & (1 << 3)) kprintf("Command/Data -> PS/2 device.\t");
    else kprintf("Command/Data -> PS/2 controller.\t");
    if (status & (1 << 6)) kprintf("Timeout error.\t");
    else kprintf("No timeout error.\t");
    if (status & (1 << 7)) kprintf("Parity error. ");
    else kprintf("No parity error.");
    kprintf(")\n");
    
    i386_outb(0x64, 0xAA);
    uint8_t result = i386_inb(0x60);
    if (result == 0x55) kprintf("PS/2 controller test passed.\n");
    else if (result == 0xFC) kprintf("PS/2 controller test failed.\n");
    else {
        kprintf("PS/2 controller responded to test with unknown code %x\n", result);
        kprintf("Trying to continue.\n");
    }

    i386_outb(0x64, 0x20);
    result = i386_inb(0x60);
    kprintf("PS/2 config byte: %x\n", result);

    i386_irq_registerHandler(1, i386_keyboard_handler);
}

void _enable_keyboard() {
    _curr_char = EMPTY_KEYBOARD;
}

char pop_character() {
    char character = _curr_char;
    _curr_char = EMPTY_KEYBOARD;
    return character;
}

void i386_keyboard_handler(struct Registers* regs) {
    char character = i386_inb(0x60);
    if (character < 0 || character >= 128) return;

    _keyboard_data.key_pressed[(int)character] = false;
    if (!(character & 0x80)) {
        _keyboard_data.key_pressed[(int)character] = true;
        _curr_char = _alphabet[(int)character];
        if (_keyboard_data.key_pressed[LSHIFT] || _keyboard_data.key_pressed[RSHIFT]) _curr_char = _shift_alphabet[(int)character];
        if (_curr_char == LSHIFT_BUTTON || _curr_char == RSHIFT_BUTTON) return;
    }

    if (_keyboard_data.key_pressed[LSHIFT] || _keyboard_data.key_pressed[RSHIFT]) {
        _keyboard_data.key_pressed[LSHIFT] = false;
        _keyboard_data.key_pressed[RSHIFT] = false;
    }
}

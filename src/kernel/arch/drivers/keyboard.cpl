extern function kprintf(ptr i8 fmt, ...) -> i0;
extern function i386_inb(u16 port) -> i8;
extern function i386_outb(u16 port, u8 data) -> i0;
extern function i386_irq_registerHandler(i32 irq, ptr i0 handler) -> i0;

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

glob arr _alphabet[128, u8] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',
    '9', '0', '-', '=', '\b',
    '\t',
    'q', 'w', 'e', 'r',
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', ENTER_BUTTON,
    0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', LSHIFT_BUTTON,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n',
    'm', ',', '.', '/',  RSHIFT_BUTTON,
    '*',
    0,
    ' ',
    '\5',
    F1_BUTTON,
    F2_BUTTON,   F3_BUTTON,   F4_BUTTON,   0,   0,   0,   0,   0,
    0,
    0,
    0,
    0,
    UP_ARROW_BUTTON,
    0,
    '-',
    LEFT_ARROW_BUTTON,
    0,
    RIGHT_ARROW_BUTTON,
    '+',
    0,
    DOWN_ARROW_BUTTON,
    0,
    0,
    DEL_BUTTON,
    0,   0,   0,
    0,
    0,
    0
};

glob arr _shift_alphabet[128, u8] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*',
    '(', ')', '_', '+', '\b',
    '\t',
    'Q', 'W', 'E', 'R',
    'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', ENTER_BUTTON,
    0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    '"', '~', LSHIFT_BUTTON,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N',
    'M', '<', '>', '?',  RSHIFT_BUTTON,
    '*',
    0,
    ' ',
    '\5',
    F1_BUTTON,
    F2_BUTTON, F3_BUTTON, F4_BUTTON, 0, 0, 0, 0, 0,
    0,
    0,
    0,
    0,
    UP_ARROW_BUTTON,
    0,
    '-',
    LEFT_ARROW_BUTTON,
    0,
    RIGHT_ARROW_BUTTON,
    '+',
    0,
    DOWN_ARROW_BUTTON,
    0,
    0,
    DEL_BUTTON,
    0,   0,   0,
    0,
    0,
    0
};

glob i8 _curr_char = EMPTY_KEYBOARD;
glob arr _key_pressed[128, u8] = { 0 };

function _keyboard_wait_input_clear() -> i8 {
    i32 timeout = 100000;
    while i386_inb(KBD_STATUS_PORT) & KBD_STATUS_INPUT_FULL; {
        timeout -= 1;
        if timeout <= 0; return 0;
    }

    return 1;
}

function _keyboard_wait_output_full() -> i32 {
    i32 timeout = 100000;
    while not (i386_inb(KBD_STATUS_PORT) & KBD_STATUS_OUTPUT_FULL); {
        timeout -= 1;
        if timeout <= 0; return 0;
    }

    return 1;
}

function _keyboard_write_command(u8 command) -> i32 {
    if not _keyboard_wait_input_clear(); return 0;
    i386_outb(KBD_COMMAND_PORT, command);
    return 1;
}

function _keyboard_write_data(u8 data) -> i8 {
    if not _keyboard_wait_input_clear(); return 0;
    i386_outb(KBD_DATA_PORT, data);
    return 1;
}

function _keyboard_read_data(ptr u8 data) -> i8 {
    if not _keyboard_wait_output_full(); return 0;
    dref data = i386_inb(KBD_DATA_PORT);
    return 1;
}

function _keyboard_flush_output() -> i0 {
    i32 timeout = 32;
    while (i386_inb(KBD_STATUS_PORT) & KBD_STATUS_OUTPUT_FULL) && (timeout > 0); {
        timeout -= 1;
        i386_inb(KBD_DATA_PORT);
    }
}

function i386_keyboard_handler(ptr i0 _) -> i0{
    u8 scancode = i386_inb(0x60);
    u8 key = scancode & 0x7F;

    if scancode & 0x80; {
        _key_pressed[key as i32] = 0;
        return;
    }

    _key_pressed[key as i32] = 1;
    _curr_char = _alphabet[key as i32];

    if _key_pressed[LSHIFT] || _key_pressed[RSHIFT];
        _curr_char = _shift_alphabet[key as i32];
}

glob function i386_init_keyboard() -> i0 {
    _keyboard_flush_output();

    u8 status = i386_inb(KBD_STATUS_PORT);
    kprintf(ref "[KEYBOARD INFO]: ( ");
    if status & (1 << 0); kprintf(ref "Output buffer full.\t");
    else kprintf(ref "Output buffer empty.\t");
    if status & (1 << 1); kprintf(ref "Input buffer full.\t");
    else kprintf(ref "Input buffer empty.\t");
    if status & (1 << 2); kprintf(ref "System flag set.\t");
    else kprintf(ref "System flag unset.\t");
    if status & (1 << 3); kprintf(ref "Command/Data -> PS/2 device.\t");
    else kprintf(ref "Command/Data -> PS/2 controller.\t");
    if status & (1 << 6); kprintf(ref "Timeout error.\t");
    else kprintf(ref "No timeout error.\t");
    if status & (1 << 7); kprintf(ref "Parity error. ");
    else kprintf(ref "No parity error.");
    kprintf(ref ")\n");

    u8 config = 0;
    if 
        not _keyboard_write_command(KBD_CMD_READ_CONFIG) || 
        not _keyboard_read_data(ref config); 
    kprintf(ref "PS/2 config byte unavailable, trying to continue.\n");
    else {
        kprintf(ref "PS/2 config byte: %x\n", config);
        config |= KBD_CONFIG_IRQ1;
        config |= 0x40;
        config &= 0xFF ^ KBD_CONFIG_PORT1_CLOCK;
        if _keyboard_write_command(KBD_CMD_WRITE_CONFIG); {
            _keyboard_write_data(config);
        }
    }

    _keyboard_write_command(KBD_CMD_ENABLE_PORT1);
    _keyboard_flush_output();

    i386_irq_registerHandler(1, i386_keyboard_handler);
}

glob function enable_keyboard() -> i0 {
    _curr_char = EMPTY_KEYBOARD;
}

glob function pop_character() -> i8 {
    i8 character = _curr_char;
    _curr_char = EMPTY_KEYBOARD;
    return character;
}
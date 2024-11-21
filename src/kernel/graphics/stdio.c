#include "../include/stdio.h"


static const char _HexChars[] = "0123456789ABCDEF";
stdio_mode_t KSTDIO_data = {
    .clrscr         = VGA_clrscr,
    .fill_color     = VGA_set_color,
    .putc           = VGA_putc,
    .get_cursor_x   = VGA_cursor_get_x,
    .get_cursor_y   = VGA_cursor_get_y,
    .set_cursor     = VGA_setcursor,
    .put_chr        = VGA_putchr,
    .get_char       = VGA_getchr
};


void kclrscr() {
    KSTDIO_data.clrscr();
}

void kputc(char c) {
    _kfputc(c);
}

void kputs(const char* str) {
    _kfputs(str);
}

void kprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    kvfprintf(fmt, args);
    va_end(args);
}

void kprint_buffer(const char* msg, const void* buffer, uint32_t count) {
    _kfprint_buffer(msg, buffer, count);
}

void kprint_hex_table(const char* data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        kprintf("%c%c ", '0' + ((unsigned char)*data >> 4), '0' + ((unsigned char)*data & 0x0F));
        if ((i + 1) % 16 == 0 || i == length - 1) kprintf("\n");
        data++;
    }
}

void kset_color(int color) {
    KSTDIO_data.fill_color(color);
}

void kfprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    kvfprintf(fmt, args);
    va_end(args);
}

void kvfprintf(const char* fmt, va_list args) {
    int state   = PRINTF_STATE_NORMAL;
    int length  = PRINTF_LENGTH_DEFAULT;
    int radix   = 10;

    bool sign   = false;
    bool number = false;

    while (*fmt) {
        switch (state) {
            case PRINTF_STATE_NORMAL:
                switch (*fmt) {
                    case '%':   
                        state = PRINTF_STATE_LENGTH;
                    break;

                    default:    
                        _kfputc(*fmt);
                    break;
                }

                break;

            case PRINTF_STATE_LENGTH:
                switch (*fmt) {
                    case 'h':   
                        length  = PRINTF_LENGTH_SHORT;  
                        state   = PRINTF_STATE_LENGTH_SHORT;
                    break;

                    case 'l':   
                        length  = PRINTF_LENGTH_LONG;
                        state   = PRINTF_STATE_LENGTH_LONG;
                    break;

                    default:    
                        goto PRINTF_STATE_SPEC_;
                }

                break;

            case PRINTF_STATE_LENGTH_SHORT:
                if (*fmt == 'h') {
                    length = PRINTF_LENGTH_SHORT_SHORT;
                    state  = PRINTF_STATE_SPEC;
                }
                else 
                    goto PRINTF_STATE_SPEC_;

                break;

            case PRINTF_STATE_LENGTH_LONG:
                if (*fmt == 'l') {
                    length  = PRINTF_LENGTH_LONG_LONG;
                    state   = PRINTF_STATE_SPEC;
                }
                else 
                    goto PRINTF_STATE_SPEC_;

                break;

            case PRINTF_STATE_SPEC:
            PRINTF_STATE_SPEC_:
                switch (*fmt) {
                    case 'c':   
                        _kfputc((char)va_arg(args, int));
                    break;

                    case 's':   
                        _kfputs(va_arg(args, const char*));
                    break;

                    case '%':   
                        _kfputc('%');
                    break;

                    case 'd':
                    case 'i':   
                        radix   = 10; 
                        sign    = true; 
                        number  = true;
                    break;

                    case 'u':   
                        radix   = 10; 
                        sign    = false; 
                        number  = true;
                    break;

                    case 'X':
                    case 'x':
                    case 'p':   
                        radix   = 16; 
                        sign    = false; 
                        number  = true;
                    break;

                    case 'o':   
                        radix   = 8; 
                        sign    = false; 
                        number  = true;
                    break;

                    default: break;
                }

                if (number) {
                    if (sign) {
                        switch (length) {
                            case PRINTF_LENGTH_SHORT_SHORT:
                            case PRINTF_LENGTH_SHORT:
                            case PRINTF_LENGTH_DEFAULT:     
                                _kfprintf_signed(va_arg(args, int), radix);
                            break;

                            case PRINTF_LENGTH_LONG:        
                                _kfprintf_signed(va_arg(args, long), radix);
                            break;

                            case PRINTF_LENGTH_LONG_LONG:   
                                _kfprintf_signed(va_arg(args, long long), radix);
                            break;
                        }
                    }
                    else {
                        switch (length) {
                            case PRINTF_LENGTH_SHORT_SHORT:
                            case PRINTF_LENGTH_SHORT:
                            case PRINTF_LENGTH_DEFAULT:
                                _kfprintf_unsigned(va_arg(args, unsigned int), radix);
                            break;
                                                            
                            case PRINTF_LENGTH_LONG:
                                _kfprintf_unsigned(va_arg(args, unsigned  long), radix);
                            break;

                            case PRINTF_LENGTH_LONG_LONG:
                                _kfprintf_unsigned(va_arg(args, unsigned  long long), radix);
                            break;
                        }
                    }
                }

                // reset state
                state  = PRINTF_STATE_NORMAL;
                length = PRINTF_LENGTH_DEFAULT;
                radix  = 10;
                sign   = false;
                number = false;

                break;
        }

        fmt++;
    }
}

void _kfputc(char c) {
    KSTDIO_data.putc(c);
}

void _kfputs(const char* str) {
    while(*str) {
        _kfputc(*str);
        str++;
    }
}

void _kfprint_buffer(const char* msg, const void* buffer, uint32_t count) {
    const uint8_t* u8Buffer = (const uint8_t*)buffer;
    
    _kfputs(msg);
    for (uint16_t i = 0; i < count; i++) {
        _kfputc(_HexChars[u8Buffer[i] >> 4]);
        _kfputc(_HexChars[u8Buffer[i] & 0xF]);
        _kfputc(' ');
    }

    _kfputc('\n');
}

void _kfprintf_unsigned(unsigned long long number, int radix) {
    char buffer[32] = { 0 };
    int pos = 0;

    // convert number to ASCII
    do {
        unsigned long long rem = number % radix;
        number /= radix;
        buffer[pos++] = _HexChars[rem];
    } while (number > 0);

    // print number in reverse order
    while (--pos >= 0) _kfputc(buffer[pos]);
}

void _kfprintf_signed(long long number, int radix) {
    if (number < 0) {
        _kfputc('-');
        _kfprintf_unsigned(-number, radix);
    }
    else _kfprintf_unsigned(number, radix);
}

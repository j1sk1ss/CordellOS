#include "../include/kstdio.h"


static const char _hex_chars[] = "0123456789ABCDEF";
stdio_mode_t KSTDIO_data = {
    .clrscr         = NULL,
    .fill_color     = NULL,
    .putc           = NULL,
    .get_cursor_x   = NULL,
    .get_cursor_y   = NULL,
    .set_cursor     = NULL,
    .put_chr        = NULL,
    .get_char       = NULL
};


void _print_unsigned(unsigned long long number, int radix) {
    char buffer[32] = { 0 };
    int pos = 0;

    // convert number to ASCII
    do {
        unsigned long long rem = number % radix;
        number /= radix;
        buffer[pos++] = _hex_chars[rem];
    } while (number > 0);

    // print number in reverse order
    while (--pos >= 0) kputc(buffer[pos]);
}

void _print_signed(long long number, int radix) {
    if (number >= 0) _print_unsigned(number, radix);
    else {
        kputc('-');
        _print_unsigned(-number, radix);
    }
}

void _kvfprintf(const char* fmt, va_list args) {
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
                        kputc(*fmt);
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
                        kputc((char)va_arg(args, int));
                    break;

                    case 's':   
                        kputs(va_arg(args, const char*));
                    break;

                    case '%':   
                        kputc('%');
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
                                _print_signed(va_arg(args, int), radix);
                            break;

                            case PRINTF_LENGTH_LONG:        
                                _print_signed(va_arg(args, long), radix);
                            break;

                            case PRINTF_LENGTH_LONG_LONG:   
                                _print_signed(va_arg(args, long long), radix);
                            break;
                        }
                    }
                    else {
                        switch (length) {
                            case PRINTF_LENGTH_SHORT_SHORT:
                            case PRINTF_LENGTH_SHORT:
                            case PRINTF_LENGTH_DEFAULT:
                                _print_unsigned(va_arg(args, unsigned int), radix);
                            break;
                                                            
                            case PRINTF_LENGTH_LONG:
                                _print_unsigned(va_arg(args, unsigned  long), radix);
                            break;

                            case PRINTF_LENGTH_LONG_LONG:
                                _print_unsigned(va_arg(args, unsigned  long long), radix);
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

void kclrscr() {
    KSTDIO_data.clrscr();
}

void kputc(char c) {
    KSTDIO_data.putc(c);
}

void kputs(const char* str) {
    while(*str) {
        kputc(*str);
        str++;
    }
}

void kprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _kvfprintf(fmt, args);
    va_end(args);
}

void kset_color(int color) {
    KSTDIO_data.fill_color(color);
}
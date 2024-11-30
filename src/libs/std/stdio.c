#include "../include/stdio.h"


static int _cursor_x = 0;
static int _cursor_y = 0;


char directly_getchar(int x, int y) {
    // Not implemented
}

void cursor_set32(uint32_t x, uint32_t y) {
    _cursor_x = x;
    _cursor_y = y;
}

void cursor_get_x32() {
    return _cursor_x;
}

void cursor_get_y32() {
    return _cursor_y;
}

void clrscr() {
    set_color(BLACK, 0, 0, get_resolution_x(), get_resolution_y());
}

void __scrollback(int lines) {
    scroll(lines);
    set_color(BLACK, 0, get_resolution_x() - lines, get_resolution_x(), get_resolution_y());
}

void __newline() {
    _cursor_x = 0;
    if (_cursor_y < get_resolution_y()) _cursor_y += _psf_get_height(get_font());
    else {
        __scrollback(_psf_get_height(get_font()));
        _cursor_y = get_resolution_x() - _psf_get_height(get_font());
    }
}

void putc(char c, uint32_t fcolor, uint32_t bcolor) {
    int _tabSize = 4;
    if (_cursor_x + _psf_get_width(get_font()) >= get_resolution_x()) __newline();
    switch (c) {
        case '\n':
            __newline();
        break;

        case '\t':
            for (int i = 0; i < _tabSize - ((get_resolution_x()) / _psf_get_width(get_font()) % _tabSize); i++)
                display_char(_cursor_x, _cursor_y, ' ', fcolor, bcolor);
            break;

        default:
            display_char(_cursor_x, _cursor_y, c, fcolor, bcolor);
            _cursor_x += _psf_get_width(get_font());
        break;
    }
}

void puts(const char* str, uint32_t fcolor, uint32_t bcolor) {
    while(*str) {
        putc(*str, fcolor, bcolor);
        str++;
    }
}

void set_color(uint32_t color, int start_x, int start_y, int end_x, int end_y) {
    for (int i = start_x; i < end_x; i++)
        for (int j = start_y; j < end_y; j++)
            vput_pixel(i, j, color);

    swipe_buffers();
}

void _fprintf_unsigned(unsigned long long number, int radix, uint32_t fcolor, uint32_t bcolor) {
    char hexChars[17] = "0123456789ABCDEF";
    char buffer[32] = { 0 };
    int pos = 0;

    // convert number to ASCII
    do {
        unsigned long long rem = number % radix;
        number /= radix;
        buffer[pos++] = hexChars[rem];
    } while (number > 0);

    // print number in reverse order
    while (--pos >= 0) putc(buffer[pos], fcolor, bcolor);
}

void _fprintf_signed(long long number, int radix, uint32_t fcolor, uint32_t bcolor) {
    if (number >= 0) _fprintf_unsigned(number, radix, fcolor, bcolor);
    else {
        putc('-', fcolor, bcolor);
        _fprintf_unsigned(-number, radix, fcolor, bcolor);
    }
}

int _vsprintf_unsigned(char* buffer, unsigned long long number, int radix, int position) {
    char hexChars[17] = "0123456789ABCDEF";
    char numBuffer[32] = { 0 };
    int pos = 0;

    do  {
        unsigned long long rem = number % radix;
        number /= radix;
        numBuffer[pos++] = hexChars[rem];
    } while (number > 0);

    while (--pos >= 0) buffer[position++] = numBuffer[pos];
    return position;
}

int _vsprintf_signed(char* buffer, long long number, int radix, int position) {
    if (number >= 0) _vsprintf_unsigned(buffer, number, radix, position);
    else {
        buffer[position++] = '-';
        position = _vsprintf_unsigned(buffer, -number, radix, position);
    }

    return position;
}

void _vsprintf(
    int type, char* buffer, int len, const char* fmt, uint32_t fcolor, uint32_t bcolor, va_list args
) {
    int state   = PRINTF_STATE_NORMAL;
    int length  = PRINTF_LENGTH_DEFAULT;
    int radix   = 10;
    bool sign   = false;
    bool number = false;
    int pos     = 0;

    while (*fmt && pos < len) {
        if (state == PRINTF_STATE_NORMAL) {
            switch (*fmt) {
                case '%':   
                    state = PRINTF_STATE_LENGTH;
                break;

                default:
                    if (type == STDOUT) putc(*fmt, fcolor, bcolor);
                    else buffer[pos++] = *fmt;
                break;
            }
        }

        else if (state == PRINTF_STATE_LENGTH) {
            switch (*fmt) {
                case 'h':   
                    length  = PRINTF_LENGTH_SHORT;  
                    state   = PRINTF_STATE_LENGTH_SHORT;
                break;

                case 'l':   
                    length  = PRINTF_LENGTH_LONG;
                    state   = PRINTF_STATE_LENGTH_LONG;
                break;

                default: goto PRINTF_STATE_SPEC_;
            }
        }

        else if (state == PRINTF_STATE_LENGTH_SHORT) {
            if (*fmt == 'h') {
                length  = PRINTF_LENGTH_SHORT_SHORT;
                state   = PRINTF_STATE_SPEC;
            }
            else goto PRINTF_STATE_SPEC_;           
        }

        else if (state == PRINTF_STATE_LENGTH_LONG) {
            if (*fmt == 'l') {
                length  = PRINTF_LENGTH_LONG_LONG;
                state   = PRINTF_STATE_SPEC;
            }
            else goto PRINTF_STATE_SPEC_;            
        }

        else if (state == PRINTF_STATE_SPEC) {
            PRINTF_STATE_SPEC_:
            if (*fmt == 'c') {
                if (type == STDOUT) putc((char)va_arg(args, int), fcolor, bcolor);
                else buffer[pos] = (char)va_arg(args, int);
            }

            else if (*fmt == 's') {
                if (type == STDOUT) puts(va_arg(args, const char*), fcolor, bcolor);
                else {
                    const char* text = va_arg(args, const char*);
                    while (*text) {
                        buffer[pos++] = *text;
                        text++;
                    }
                }
            }

            else if (*fmt == '%') {
                if (type == STDOUT) putc('%', fcolor, bcolor);
                else buffer[pos] = '%';
            }
            
            else if (*fmt == 'd' || *fmt == 'i') {
                radix   = 10; 
                sign    = true; 
                number  = true;
            }

            else if (*fmt == 'u') {
                radix   = 10; 
                sign    = false; 
                number  = true;
            }

            else if (*fmt == 'X' || *fmt == 'x' || *fmt == 'p') {
                radix   = 16; 
                sign    = false; 
                number  = true;
            }

            else if (*fmt == 'o') {
                radix   = 8; 
                sign    = false; 
                number  = true;
            }

            if (number == true) {
                if (sign == true) {
                    if (
                        length == PRINTF_LENGTH_SHORT_SHORT || 
                        length == PRINTF_LENGTH_SHORT || 
                        length == PRINTF_LENGTH_DEFAULT
                    ) { 
                        if (type == STDOUT) _fprintf_signed(va_arg(args, int), radix, fcolor, bcolor); 
                        else pos = _vsprintf_signed(buffer, va_arg(args, int), radix, pos); 
                    }
                    else if (length == PRINTF_LENGTH_LONG) { 
                        if (type == STDOUT) _fprintf_signed(va_arg(args, long), radix, fcolor, bcolor); 
                        else pos = _vsprintf_signed(buffer, va_arg(args, long), radix, pos); 
                    }
                    else if (length == PRINTF_LENGTH_LONG_LONG) { 
                        if (type == STDOUT) _fprintf_signed(va_arg(args, long long), radix, fcolor, bcolor); 
                        else pos = _vsprintf_signed(buffer, va_arg(args, long long), radix, pos); 
                    }
                }
                else {
                    if (
                        length == PRINTF_LENGTH_SHORT_SHORT || 
                        length == PRINTF_LENGTH_SHORT || 
                        length == PRINTF_LENGTH_DEFAULT
                    ) { 
                        if (type == STDOUT) _fprintf_unsigned(va_arg(args, int), radix, fcolor, bcolor); 
                        else pos = _vsprintf_unsigned(buffer, va_arg(args, int), radix, pos); 
                    }
                    else if (length == PRINTF_LENGTH_LONG) { 
                        if (type == STDOUT) _fprintf_unsigned(va_arg(args, long), radix, fcolor, bcolor); 
                        else pos = _vsprintf_unsigned(buffer, va_arg(args, long), radix, pos); 
                    }
                    else if (length == PRINTF_LENGTH_LONG_LONG) { 
                        if (type == STDOUT) _fprintf_unsigned(va_arg(args, long long), radix, fcolor, bcolor); 
                        else pos = _vsprintf_unsigned(buffer, va_arg(args, long long), radix, pos); 
                    }
                }
            }

            // reset state
            state   = PRINTF_STATE_NORMAL;
            length  = PRINTF_LENGTH_DEFAULT;
            radix   = 10;
            sign    = false;
            number  = false;            
        }

        fmt++;
    }
}

void printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _vsprintf(STDOUT, NULL, 0, fmt, WHITE, BLACK, args);
    va_end(args);
}

void cprintf(uint32_t fcolor, uint32_t bcolor, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _vsprintf(STDOUT, NULL, 0, fmt, fcolor, bcolor, args);
    va_end(args);
}

void sprintf(char* buffer, int len, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _vsprintf(MEMORY, buffer, len, fmt, NO_COLOR, NO_COLOR, args);
    va_end(args);
}

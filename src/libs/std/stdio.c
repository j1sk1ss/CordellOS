#include <stdio.h>
#include <graphics.h>

static int _curr_x = 0;
static int _curr_y = 0;

void __scrollback(int lines) {
    int max_h = get_resolution_y();
    scroll(lines);
    set_pcolor(BLACK, 0, max_h - lines, get_resolution_x(), max_h);
}

void __newline() {
    int char_h = psf_get_height(get_font());
    int max_h = get_resolution_y();

    if (char_h <= 0) {
        return;
    }

    _curr_x = 0;
    if (_curr_y < max_h) _curr_y += char_h;
    else {
        __scrollback(char_h);
        _curr_y = max_h - char_h;
    }
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
    while (--pos >= 0) cputc(buffer[pos], fcolor, bcolor);
}

void _fprintf_signed(long long number, int radix, uint32_t fcolor, uint32_t bcolor) {
    if (number >= 0) _fprintf_unsigned(number, radix, fcolor, bcolor);
    else {
        cputc('-', fcolor, bcolor);
        _fprintf_unsigned(-number, radix, fcolor, bcolor);
    }
}

int _vsprintf_unsigned(char* buffer, unsigned long long number, int radix, int position) {
    char hexChars[17] = "0123456789ABCDEF";
    char numBuffer[32] = { 0 };
    int pos = 0;

    do {
        unsigned long long rem = number % radix;
        number /= radix;
        numBuffer[pos++] = hexChars[rem];
    } while (number > 0);

    while (--pos >= 0) buffer[position++] = numBuffer[pos];
    return position;
}

int _vsprintf_signed(char* buffer, long long number, int radix, int position) {
    if (number >= 0) position = _vsprintf_unsigned(buffer, number, radix, position);
    else {
        buffer[position++] = '-';
        position = _vsprintf_unsigned(buffer, -number, radix, position);
    }

    return position;
}

int _vsprintf(
    int type, char* buffer, int len, const char* fmt, uint32_t fcolor, uint32_t bcolor, va_list args
) {
    int state   = PRINTF_STATE_NORMAL;
    int length  = PRINTF_LENGTH_DEFAULT;
    int radix   = 10;
    bool sign   = false;
    bool number = false;
    int pos     = 0;

    while (*fmt && (pos < len || buffer == NULL)) {
        if (state == PRINTF_STATE_NORMAL) {
            switch (*fmt) {
                case '%':   
                    state = PRINTF_STATE_LENGTH;
                break;

                default:
                    if (type == STDOUT) cputc(*fmt, fcolor, bcolor);
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
                if (type == STDOUT) cputc((char)va_arg(args, int), fcolor, bcolor);
                else buffer[pos++] = (char)va_arg(args, int);
            }

            else if (*fmt == 's') {
                if (type == STDOUT) cputs(va_arg(args, const char*), fcolor, bcolor);
                else {
                    const char* text = va_arg(args, const char*);
                    while (*text) {
                        buffer[pos++] = *text;
                        text++;
                    }
                }
            }
            else if (*fmt == '%') {
                if (type == STDOUT) cputc('%', fcolor, bcolor);
                else buffer[pos++] = '%';
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

    if (buffer != NULL && len > 0) {
        buffer[pos < len ? pos : len - 1] = '\0';
    }

    return pos;
}

void cursor_set32(uint32_t x, uint32_t y) {
    _curr_x = x;
    _curr_y = y;
}

uint32_t cursor_get_x32() {
    return _curr_x;
}

uint32_t cursor_get_y32() {
    return _curr_y;
}

void clrscr() {
    set_vcolor(BLACK, 0, 0, get_resolution_x(), get_resolution_y());
    cursor_set32(0, 0);
    swipe_buffers();
}

void cputc(char c, uint32_t fcolor, uint32_t bcolor) {
    int char_w = psf_get_width(get_font());
    int max_w = get_resolution_x();

    if (char_w <= 0) {
        return;
    }

    if (_curr_x + char_w >= max_w) __newline();
    switch (c) {
        case '\n':
            __newline();
        break;

        case '\t':
            for (int i = 0; i < 4 - ((max_w) / char_w % 4); i++)
                cputc(' ', fcolor, bcolor);
            break;

        default:
            display_char(_curr_x, _curr_y, c, fcolor, bcolor);
            _curr_x += char_w;
        break;
    }
}

void cputs(const char* str, uint32_t fcolor, uint32_t bcolor) {
    while (*str) {
        cputc(*str, fcolor, bcolor);
        str++;
    }
}

int putchar(int c) {
    cputc((char)c, WHITE, BLACK);
    return c;
}

int puts(const char* str) {
    cputs(str, WHITE, BLACK);
    cputc('\n', WHITE, BLACK);
    return 0;
}

void set_pcolor(uint32_t color, int start_x, int start_y, int end_x, int end_y) {
    for (int i = start_x; i < end_x; i++)
        for (int j = start_y; j < end_y; j++)
            pput_pixel(i, j, color);
}

void set_vcolor(uint32_t color, int start_x, int start_y, int end_x, int end_y) {
    for (int i = start_x; i < end_x; i++)
        for (int j = start_y; j < end_y; j++)
            vput_pixel(i, j, color);
}

int printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = _vsprintf(STDOUT, NULL, 0, fmt, WHITE, BLACK, args);
    va_end(args);
    return result;
}

int cprintf(uint32_t fcolor, uint32_t bcolor, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = _vsprintf(STDOUT, NULL, 0, fmt, fcolor, bcolor, args);
    va_end(args);
    return result;
}

int vsnprintf(char* buffer, size_t len, const char* fmt, va_list args) {
    return _vsprintf(MEMORY, buffer, (int)len, fmt, NO_COLOR, NO_COLOR, args);
}

int snprintf(char* buffer, size_t len, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = vsnprintf(buffer, len, fmt, args);
    va_end(args);
    return result;
}

int sprintf(char* buffer, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = vsnprintf(buffer, 0x7fffffff, fmt, args);
    va_end(args);
    return result;
}

int vfprintf(FILE* stream, const char* fmt, va_list args) {
    (void)stream;
    return _vsprintf(STDOUT, NULL, 0, fmt, WHITE, BLACK, args);
}

static int _scan_int(const char** input, int base, int* output) {
    const char* str = *input;
    int sign = 1;
    int value = 0;
    int digits = 0;

    while (*str == ' ' || *str == '\t' || *str == '\n') {
        str++;
    }

    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    if ((base == 0 || base == 16) && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        base = 16;
        str += 2;
    } else if (base == 0 && str[0] == '0') {
        base = 8;
        str++;
    } else if (base == 0) {
        base = 10;
    }

    while (*str) {
        int digit = -1;
        if (*str >= '0' && *str <= '9') {
            digit = *str - '0';
        } else if (*str >= 'a' && *str <= 'f') {
            digit = *str - 'a' + 10;
        } else if (*str >= 'A' && *str <= 'F') {
            digit = *str - 'A' + 10;
        }

        if (digit < 0 || digit >= base) {
            break;
        }

        value = value * base + digit;
        digits++;
        str++;
    }

    if (digits == 0) {
        return 0;
    }

    *output = value * sign;
    *input = str;
    return 1;
}

int sscanf(const char* str, const char* fmt, ...) {
    va_list args;
    int assigned = 0;

    va_start(args, fmt);

    while (*fmt) {
        if (*fmt == ' ' || *fmt == '\t' || *fmt == '\n') {
            while (*str == ' ' || *str == '\t' || *str == '\n') {
                str++;
            }
            fmt++;
            continue;
        }

        if (*fmt != '%') {
            if (*str != *fmt) {
                break;
            }
            str++;
            fmt++;
            continue;
        }

        fmt++;
        int* output = va_arg(args, int*);
        int base = 10;
        if (*fmt == 'x' || *fmt == 'X') {
            base = 16;
        } else if (*fmt == 'o') {
            base = 8;
        } else if (*fmt == 'i') {
            base = 0;
        } else if (*fmt != 'd') {
            break;
        }

        if (!_scan_int(&str, base, output)) {
            break;
        }

        assigned++;
        fmt++;
    }

    va_end(args);
    return assigned;
}

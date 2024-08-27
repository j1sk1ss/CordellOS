#include "../include/stdio.h"

//==================================
//           FUNCTIONS
//==================================

//====================================================================
// Function directly in screen put color by coordinates
// EBX - x
// ECX - y
// EDX - color
void directly_putclr(int x, int y, uint32_t color) {
    __asm__ volatile(
        "movl $24, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "movl %2, %%edx\n"
        "int %3\n"
        :
        : "r"(x), "r"(y), "r"((int)color), "i"(SYSCALL_INTERRUPT)
        : "eax", "ebx", "ecx"
    );
}

//====================================================================
// Function directly in screen put character by coordinates
// EBX - x
// ECX - y
// EDX - character
void directly_putc(int x, int y, char character) {
    __asm__ volatile(
        "movl $23, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "movl %2, %%edx\n"
        "int %3\n"
        :
        : "r"(x), "r"(y), "r"((int)character), "i"(SYSCALL_INTERRUPT)
        : "eax", "ebx", "ecx"
    );
}

//====================================================================
// Function get character from screen by coordinates
// EBX - x
// ECX - y
// AL - result
char directly_getchar(int x, int y) {
    char result;
    __asm__ volatile(
        "movl $22, %%eax\n"
        "movl %1, %%ebx\n"
        "movl %2, %%ecx\n"
        "int %3\n"
        "movb %%al, %0\n"
        : "=r"(result)
        : "r"(x), "r"(y), "i"(SYSCALL_INTERRUPT)
        : "eax", "ebx", "ecx"
    );

    return result;
}

//====================================================================
// Function set cursor by char coordinates (x * CHAR SIZE)
// EBX - x
// ECX - y
void cursor_set(int x, int y) {
    __asm__ volatile(
        "movl $20, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "int %2\n"
        :
        : "r"(x), "r"(y), "i"(SYSCALL_INTERRUPT)
        : "eax", "ebx", "ecx"
    );
}

//====================================================================
// Function set cursor by global coordinates
// EBX - x
// ECX - y
void cursor_set32(uint32_t x, uint32_t y) {
    __asm__ volatile(
        "movl $47, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "int %2\n"
        :
        : "r"(x), "r"(y), "i"(SYSCALL_INTERRUPT)
        : "eax", "ebx", "ecx"
    );
}

//====================================================================
// Function get coordinates of cursor
// ECX - pointer to result array
// 
// result[0] - x
// result[1] - y
void cursor_get(int* result) {
    __asm__ volatile(
        "mov $21, %%rax\n"
        "mov %0, %%rcx\n"
        "int %1\n"
        :
        : "r"(result), "i"(SYSCALL_INTERRUPT)
        : "eax", "ebx", "ecx", "edx"
    );
}

//====================================================================
//  Clear entire screen (used kernel printf commands)
void clrscr() {
    __asm__ volatile (
        "movl $2, %%eax\n"
        "int %0\n"
        :
        : "i"(SYSCALL_INTERRUPT)
        : "eax", "ebx", "ecx", "edx"
    );
}

//====================================================================
//  Puts a char in screen (used kernel printf commands)
//  ECX - character
void fputc(char c, uint32_t file, int color) {
    if (color == 0) {
        __asm__ volatile(
            "movl $1, %%eax\n"
            "movl %0, %%ecx\n"
            "int %1\n"
            :
            : "r"((int)c), "i"(SYSCALL_INTERRUPT)
            : "eax", "ebx", "ecx", "edx"
        );
    }
    else cputc(c, file);
}

//====================================================================
//  Puts a char in screen with color (used kernel printf commands)
//  ECX - color
//  EBX - character
void cputc(char c, uint32_t color) {
    __asm__ volatile(
        "movl $13, %%eax\n"
        "movl %1, %%ebx\n"
        "movl %0, %%ecx\n"
        "int %2\n"
        :
        : "r"((int)color), "r"((int)c), "i"(SYSCALL_INTERRUPT)
        : "eax", "ebx", "ecx", "edx"
    );
}

void fputs(const char* str, uint32_t file, int color) {
    while(*str) {
        fputc(*str, file, color);
        str++;
    }
}

//====================================================================
//  Fill screen by selected color (Not VBE color)
//  ECX - color
void set_color(int color) {
    __asm__ volatile(
        "movl $14, %%eax\n"
        "movl %0, %%ecx\n"
        "int %1\n"
        :
        : "r"((int)color), "i"(SYSCALL_INTERRUPT)
        : "eax", "ebx", "ecx", "edx"
    );
}

void fprintf_unsigned(uint32_t file, unsigned long long number, int radix, int color) {
    char hexChars[17] = "0123456789ABCDEF";
    char buffer[32];
    int pos = 0;

    // convert number to ASCII
    do  {
        unsigned long long rem = number % radix;
        number /= radix;
        buffer[pos++] = hexChars[rem];
    } while (number > 0);

    // print number in reverse order
    while (--pos >= 0)
        fputc(buffer[pos], file, color);
}

void fprintf_signed(uint32_t file, long long number, int radix, int color) {
    if (number < 0) {
        fputc('-', file, color);
        fprintf_unsigned(file, -number, radix, color);
    }
    else fprintf_unsigned(file, number, radix, color);
}

void vfprintf(uint32_t file, const char* fmt, va_list args, int color) {
    int state   = PRINTF_STATE_NORMAL;
    int length  = PRINTF_LENGTH_DEFAULT;
    int radix   = 10;
    bool sign   = false;
    bool number = false;

    while (*fmt) {
        if (state == PRINTF_STATE_NORMAL) {
            switch (*fmt) {
                case '%':   
                    state = PRINTF_STATE_LENGTH;
                break;

                default:    
                    fputc(*fmt, file, color);
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

                default:    
                    goto PRINTF_STATE_SPEC_;
            }
        }

        else if (state == PRINTF_STATE_LENGTH_SHORT) {
            if (*fmt == 'h') {
                length  = PRINTF_LENGTH_SHORT_SHORT;
                state   = PRINTF_STATE_SPEC;
            }
            else 
                goto PRINTF_STATE_SPEC_;           
        }

        else if (state == PRINTF_STATE_LENGTH_LONG) {
            if (*fmt == 'l') {
                length  = PRINTF_LENGTH_LONG_LONG;
                state   = PRINTF_STATE_SPEC;
            }
            else 
                goto PRINTF_STATE_SPEC_;            
        }

        else if (state == PRINTF_STATE_SPEC) {
            PRINTF_STATE_SPEC_:
            if (*fmt == 'c') fputc((char)va_arg(args, int), file, color);
            else if (*fmt == 's') fputs(va_arg(args, const char*), file, color);
            else if (*fmt == '%') fputc('%', file, color);
            
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
                    if (length == PRINTF_LENGTH_SHORT_SHORT || length == PRINTF_LENGTH_SHORT || length == PRINTF_LENGTH_DEFAULT) 
                        fprintf_signed(file, va_arg(args, int), radix, color);
                    
                    else if (length == PRINTF_LENGTH_LONG)
                        fprintf_signed(file, va_arg(args, long), radix, color);

                    else if (length == PRINTF_LENGTH_LONG_LONG)
                        fprintf_signed(file, va_arg(args, long long), radix, color);
                }
                else {
                    if (length == PRINTF_LENGTH_SHORT_SHORT || length == PRINTF_LENGTH_SHORT || length == PRINTF_LENGTH_DEFAULT) 
                        fprintf_unsigned(file, va_arg(args, unsigned int), radix, color);
                    
                    else if (length == PRINTF_LENGTH_LONG)
                        fprintf_unsigned(file, va_arg(args, unsigned long), radix, color);

                    else if (length == PRINTF_LENGTH_LONG_LONG)
                        fprintf_unsigned(file, va_arg(args, unsigned long long), radix, color);
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

int vsprintf_unsigned(char* buffer, unsigned long long number, int radix, int position) {
    char hexChars[17] = "0123456789ABCDEF";
    char numBuffer[32];
    int pos = 0;

    do  {
        unsigned long long rem = number % radix;
        number /= radix;
        numBuffer[pos++] = hexChars[rem];
    } while (number > 0);

    while (--pos >= 0)
        buffer[position++] = numBuffer[pos];

    return position;
}

int vsprintf_signed(char* buffer, long long number, int radix, int position) {
    if (number < 0) {
        buffer[position++] = '-';
        position = vsprintf_unsigned(buffer, -number, radix, position);
    }
    else position = vsprintf_unsigned(buffer, number, radix, position);
    return position;
}

void vsprintf(char* buffer, int len, const char* fmt, va_list args) {
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
                    buffer[pos++] = *fmt;
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

                default:    
                    goto PRINTF_STATE_SPEC_;
            }
        }

        else if (state == PRINTF_STATE_LENGTH_SHORT) {
            if (*fmt == 'h') {
                length  = PRINTF_LENGTH_SHORT_SHORT;
                state   = PRINTF_STATE_SPEC;
            }
            else 
                goto PRINTF_STATE_SPEC_;           
        }

        else if (state == PRINTF_STATE_LENGTH_LONG) {
            if (*fmt == 'l') {
                length  = PRINTF_LENGTH_LONG_LONG;
                state   = PRINTF_STATE_SPEC;
            }
            else 
                goto PRINTF_STATE_SPEC_;            
        }

        else if (state == PRINTF_STATE_SPEC) {
            PRINTF_STATE_SPEC_:
            if (*fmt == 'c') buffer[pos] = (char)va_arg(args, int);
            else if (*fmt == 's') {
                const char* text = va_arg(args, const char*);
                while (*text) {
                    buffer[pos++] = *text;
                    text++;
                }
            }

            else if (*fmt == '%') buffer[pos] = '%';
            
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
                    if (length == PRINTF_LENGTH_SHORT_SHORT || length == PRINTF_LENGTH_SHORT || length == PRINTF_LENGTH_DEFAULT) 
                        pos = vsprintf_signed(buffer, va_arg(args, int), radix, pos);
                    
                    else if (length == PRINTF_LENGTH_LONG)
                        pos = vsprintf_signed(buffer, va_arg(args, long), radix, pos);

                    else if (length == PRINTF_LENGTH_LONG_LONG)
                        pos = vsprintf_signed(buffer, va_arg(args, long long), radix, pos);
                }
                else {
                    if (length == PRINTF_LENGTH_SHORT_SHORT || length == PRINTF_LENGTH_SHORT || length == PRINTF_LENGTH_DEFAULT) 
                        pos = vsprintf_unsigned(buffer, va_arg(args, unsigned int), radix, pos);
                    
                    else if (length == PRINTF_LENGTH_LONG)
                        pos = vsprintf_unsigned(buffer, va_arg(args, unsigned long), radix, pos);

                    else if (length == PRINTF_LENGTH_LONG_LONG)
                        pos = vsprintf_unsigned(buffer, va_arg(args, unsigned long long), radix, pos);
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

void fprintf(uint32_t file, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(file, fmt, args, 0);
    va_end(args);
}

void fprint_buffer(const void* buffer, uint32_t count) {
    const uint8_t* u8Buffer = (const uint8_t*)buffer;
    char hexChars[17] = "0123456789ABCDEF";
    
    for (uint16_t i = 0; i < count; i++) {
        fputc(hexChars[u8Buffer[i] >> 4], 0, -1);
        fputc(hexChars[u8Buffer[i] & 0xF], 0, -1);
        fputc(' ', 0, -1);
    }

    fputc('\n', 0, -1);
}

void putc(char c) {
    fputc(c, 0, 0);
}

void puts(const char* str) {
    fputs(str, 0, 0);
}

void printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(0, fmt, args, 0);
    va_end(args);
}

void sprintf(char* buffer, int len, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, len, fmt, args);
    va_end(args);
}

void cprintf(uint32_t color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(color, fmt, args, 1);
    va_end(args);
}

void print_buffer(const void* buffer, uint32_t count) {
    fprint_buffer(buffer, count);
}

void print_hex_table(const char* data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        printf("%c%c ", '0' + ((unsigned char)data[i] >> 4), '0' + ((unsigned char)data[i] & 0x0F));
        if ((i + 1) % 16 == 0 || i == length - 1) printf("\n");
    }
}
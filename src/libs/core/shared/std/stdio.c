#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

const unsigned int SCREEN_WIDTH  = 80;      //  Default widgth of screens
const unsigned int SCREEN_HEIGHT = 25;      //  Default height of screens
const uint8_t DEFAULT_COLOR      = 0x7;     //  Default color

uint8_t* _screenBuffer = (uint8_t*)0xB8000; // Position of screen buffer in memory

int _screenX = 0;                           //  Cursor position
int _screenY = 0;                           //

//
//  Get char from screen buffer
//
char getchr(int x, int y) {
    return (char)_screenBuffer[2 * (y * SCREEN_WIDTH + x)];
}

//
//  Put character to screen buffer
//
void putchr(int x, int y, char c) {
    _screenBuffer[2 * (y * SCREEN_WIDTH + x)] = c;
}

//
//  Get char colot in current coordinates
//
uint8_t getcolor(int x, int y) {
    return _screenBuffer[2 * (y * SCREEN_WIDTH + x) + 1];
}

//
//  Put color in current coordinates
//
void putcolor(int x, int y, uint8_t color) {
    _screenBuffer[2 * (y * SCREEN_WIDTH + x) + 1] = color;
}

//
//  Set console cursore into x and y coordinates
//  X and Y between default screen widght and screen height
//  Check it on file top
//
void setcursor(int x, int y) {
    uint16_t pos = y * SCREEN_WIDTH + x;

    x86_outb(0x3D4, 0x0F);                          // First value is port on VGA, second - value 
    x86_outb(0x3D5, (uint8_t)(pos & 0xFF));         // for this register
    x86_outb(0x3D4, 0x0E);                          // Check of. docs for info about this ports
    x86_outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));  //
}

//
//  Clear screen
//
void clrscr() {
    for (int y = 0; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            putchr(x, y, 0);
            putcolor(x, y, DEFAULT_COLOR);
        }

    _screenX = 0;
    _screenY = 0;

    setcursor(0, 0);
}

//
//  Scroll all screen down
//
void scrollback(int lines) {
    for (int y = lines; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++){
            putchr(x, y - lines, getchr(x, y));
            putcolor(x, y - lines, getcolor(x, y));
        }

    for (int x = 0; x < SCREEN_WIDTH; x++){
        putchr(x, SCREEN_HEIGHT - lines, 0);
        putcolor(x, SCREEN_HEIGHT - lines, DEFAULT_COLOR);
    }

    _screenY -= lines;
}

//
//  Put character and check special symbols
//
void putc(char c) {
    const int _tabSize = 4;

    switch (c) {
        case '\n':                                          // New line
            _screenX = 0;
            _screenY++;
        break;
    
        case '\t':                                          // Tabulation
            for (int i = 0; i < _tabSize - (_screenX % _tabSize); i++)
                putc(' ');
        break;

        case '\r':                                          // Line start
            _screenX = 0;
        break;

        default:                                            // Write character
            putchr(_screenX , _screenY, c);
                _screenX += 1;
        break;
    }

    if (_screenX >= SCREEN_WIDTH) {                         // Next line when we reach the end of screen
        _screenX -= SCREEN_WIDTH;
        _screenY++;
    }

    if (_screenY >= SCREEN_HEIGHT)
        scrollback(1);

    setcursor(_screenX , _screenY);
}

void puts(const char* str) {
    while (*str){
        putc(*str);
        ++str;
    }
}

//
//  Define variables
//

//
//  Define and Include in C - Scaler Topics
//  #define is a preprocessor directive that is used to define macros in a C program.
//  #define is also known as a macros directive. #define directive is used to declare 
//  some constant values or an expression with a name that can be used throughout our C program.
//

#define PRINTF_STATE_NORMAL         0
#define PRINTF_STATE_LENGTH         1
#define PRINTF_STATE_LENGTH_SHORT   2
#define PRINTF_STATE_LENGTH_LONG    3
#define PRINTF_STATE_SPEC           4

#define PRINTF_LENGTH_DEFAULT       0
#define PRINTF_LENGTH_SHORT_SHORT   1
#define PRINTF_LENGTH_SHORT         2
#define PRINTF_LENGTH_LONG          3
#define PRINTF_LENGTH_LONG_LONG     4

//
//  Define variables
//

const char g_HexChars[] = "0123456789abcdef";

void printf_unsigned(unsigned long long number, int radix){
    char buffer[32];
    int pos = 0;

    do {
        unsigned long long rem = number % radix;
        number /= radix;
        buffer[pos++] = g_HexChars[rem];
    } while (number > 0);

    // print number in reverse order
    while (--pos >= 0)
        putc(buffer[pos]);
}

void printf_signed(long long number, int radix) {
    if (number < 0) {
        putc('-');
        printf_unsigned((unsigned long long)(-number), radix);
    }
    else {
        printf_unsigned((unsigned long long)number, radix);
    }
}

//
//  Prints char array with formating methods like %
//
void printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    int state   = PRINTF_STATE_NORMAL;
    int length  = PRINTF_LENGTH_DEFAULT;
    bool number = false;
    bool sign   = false;
    int radix   = 10;

    while (*fmt) {
        switch (state) {
            case PRINTF_STATE_NORMAL:
                switch (*fmt) {
                    case '%':   
                        state = PRINTF_STATE_LENGTH;
                    break;
                    default:    
                        putc(*fmt);
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
                    state = PRINTF_STATE_SPEC;
                }
                else goto PRINTF_STATE_SPEC_;

                break;

            case PRINTF_STATE_LENGTH_LONG:
                if (*fmt == 'l') {
                    length = PRINTF_LENGTH_LONG_LONG;
                    state = PRINTF_STATE_SPEC;
                }
                else goto PRINTF_STATE_SPEC_;

                break;

            case PRINTF_STATE_SPEC:
            PRINTF_STATE_SPEC_:
                switch (*fmt) {
                    case 'c':   
                        putc((char)va_arg(args, int));
                    break;

                    case 's':   
                        puts(va_arg(args, const char*));
                    break;

                    case '%':   
                        putc('%');
                    break;

                    case 'd':
                    case 'i':   
                        number = true; 
                        radix  = 10; 
                        sign   = true;
                    break; 

                    case 'u':   
                        number = true;
                        radix  = 10; 
                        sign   = false;
                    break;

                    case 'X':
                    case 'x':
                    case 'p':   
                        number = true; 
                        radix  = 16; 
                        sign   = false;
                    break;

                    case 'o':   
                        number = true;
                        radix  = 8;
                        sign   = false;
                    break;

                    // ignore invalid spec
                    default:    break;
                }

                // handle numbers
                if (number) {
                    if (sign)  {
                        switch (length) {
                            case PRINTF_LENGTH_SHORT_SHORT:
                            case PRINTF_LENGTH_SHORT:
                            case PRINTF_LENGTH_DEFAULT:    
                                printf_signed(va_arg(args, int), radix);
                            break;

                            case PRINTF_LENGTH_LONG:        
                                printf_signed(va_arg(args, long), radix);
                            break;

                            case PRINTF_LENGTH_LONG_LONG:   
                                printf_signed(va_arg(args, long long), radix);
                            break;
                        }
                    }
                    else {
                        switch (length) {
                            case PRINTF_LENGTH_SHORT_SHORT:
                            case PRINTF_LENGTH_SHORT:
                            case PRINTF_LENGTH_DEFAULT:     
                                printf_unsigned(va_arg(args, unsigned int), radix);
                            break;

                            case PRINTF_LENGTH_LONG:        
                                printf_unsigned(va_arg(args, unsigned long), radix);
                            break;

                            case PRINTF_LENGTH_LONG_LONG:   
                                printf_unsigned(va_arg(args, unsigned long long), radix);
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

    va_end(args);
}

//
//  Print buffer of char arrays
//
void print_buffer(const char* msg, const void* buffer, uint16_t count) {
    const uint8_t* u8Buffer = (const uint8_t*)buffer;
    
    puts(msg);
    for (uint16_t i = 0; i < count; i++) {
        putc(g_HexChars[u8Buffer[i] >> 4]);
        putc(g_HexChars[u8Buffer[i] & 0xF]);
    }
    
    puts("\n");
}
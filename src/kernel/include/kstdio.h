#ifndef KSTDIO_H_
#define KSTDIO_H_

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>


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


typedef struct stdio_mode {
    void (*clrscr)();
    void (*putc)(char);
    void (*fill_color)(uint32_t);
    uint8_t (*get_cursor_x)();
    uint8_t (*get_cursor_y)();
    void (*set_cursor)(uint8_t, uint8_t);
    void (*put_chr)(uint8_t, uint8_t, char);
    char (*get_char)(uint8_t, uint8_t);
} stdio_mode_t;


extern stdio_mode_t KSTDIO_data;


void kclrscr();

void kputc(char c);
void kputs(const char* str);
void kprintf(const char* fmt, ...);

void kset_color(int color);

#endif
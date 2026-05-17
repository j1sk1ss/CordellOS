#ifndef KSTDIO_H_
#define KSTDIO_H_

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

typedef struct stdio_mode {
    void    (*clrscr)();
    void    (*putc)(char);
    void    (*fill_color)(uint32_t);
    uint8_t (*get_cursor_x)();
    uint8_t (*get_cursor_y)();
    void    (*set_cursor)(uint8_t, uint8_t);
    void    (*put_chr)(uint8_t, uint8_t, char);
    char    (*get_char)(uint8_t, uint8_t);
} stdio_mode_t;

extern stdio_mode_t KSTDIO_data;

void kclrscr();
void kputc(char c);
void kputs(const char* str);
#define LOG(fmt, ...) kprintf("[%s.%i] %s\n", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
void kprintf(const char* fmt, ...);
void kset_color(int color);

#endif
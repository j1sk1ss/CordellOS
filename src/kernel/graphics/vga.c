#include "../include/vga.h"


vga_data_t VGA_data = {
    .width = 80,
    .height = 25,
    .color = 0x7,
    .buffer = NULL,
    .cursor_x = 0,
    .cursor_y = 0
};


void VGA_init(uint8_t* buffer) {
    KSTDIO_data.clrscr         = VGA_clrscr;
    KSTDIO_data.fill_color     = VGA_set_color;
    KSTDIO_data.putc           = VGA_putc;
    KSTDIO_data.get_cursor_x   = VGA_cursor_get_x;
    KSTDIO_data.get_cursor_y   = VGA_cursor_get_y;
    KSTDIO_data.set_cursor     = VGA_setcursor;
    KSTDIO_data.put_chr        = VGA_putchr;
    KSTDIO_data.get_char       = VGA_getchr;

    VGA_data.buffer = buffer;
}

uint8_t VGA_cursor_get_x() {
    return VGA_data.cursor_x;
}

uint8_t VGA_cursor_get_y() {
    return VGA_data.cursor_y;
}

char VGA_getchr(uint8_t x, uint8_t y) {
    return (char)VGA_data.buffer[2 * (y * VGA_data.width + x)];
}

void VGA_putchr(uint8_t x, uint8_t y, char c) {
    VGA_data.buffer[2 * (y * VGA_data.width + x)] = c;
}

uint8_t VGA_getcolor(uint8_t x, uint8_t y) {
    return VGA_data.buffer[2 * (y * VGA_data.width + x) + 1];
}

void VGA_putcolor(uint8_t x, uint8_t y, uint8_t color) {
    VGA_data.buffer[2 * (y * VGA_data.width + x) + 1] = color;
}

void VGA_setcursor(uint8_t x, uint8_t y) {
    uint16_t pos = y * VGA_data.width + x;

    i386_outb(0x3D4, 0x0F);                          // First value is port on VGA, second - value 
    i386_outb(0x3D5, (uint8_t)(pos & 0xFF));         // for this register
    i386_outb(0x3D4, 0x0E);                          // Check of. docs for info about this ports
    i386_outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));  //

    VGA_data.cursor_x = x;
    VGA_data.cursor_y = y;
}

void VGA_clrscr() {
    for (int y = 0; y < VGA_data.height; y++)
        for (int x = 0; x < VGA_data.width; x++) {
            VGA_putchr(x, y, (char)(uintptr_t)NULL);
            VGA_putcolor(x, y, VGA_data.color);
        }

    VGA_data.cursor_x = 0;
    VGA_data.cursor_y = 0;

    VGA_setcursor(VGA_data.cursor_x, VGA_data.cursor_y);
}

void VGA_set_color(uint8_t color) {
    for (int y = 0; y < VGA_data.height; y++)
        for (int x = 0; x < VGA_data.width; x++) 
            VGA_putcolor(x, y, color);
}

void VGA_scrollback(int lines) {
    for (int y = lines; y < VGA_data.height; y++)
        for (int x = 0; x < VGA_data.width; x++){
            VGA_putchr(x, y - lines, VGA_getchr(x, y));
            VGA_putcolor(x, y - lines, VGA_getcolor(x, y));
        }

    for (int x = 0; x < VGA_data.width; x++){
        VGA_putchr(x, VGA_data.height - lines, 0);
        VGA_putcolor(x, VGA_data.height - lines, VGA_data.color);
    }

    VGA_data.cursor_y -= lines;
}

void VGA_cputc(char c, uint8_t color) {
    VGA_putc(c);
    VGA_putcolor(VGA_cursor_get_x() - 1, VGA_cursor_get_y(), color);
}

void VGA_putc(char c) {
    const int _tabSize = 4;

    switch (c) {
        case '\n':                                          // New line
            VGA_data.cursor_x = 0;
            VGA_data.cursor_y++;
        break;
    
        case '\t':                                          // Tabulation
            for (int i = 0; i < _tabSize - (VGA_data.cursor_x % _tabSize); i++)
                VGA_putc(' ');
        break;

        case '\r':                                          // Line start
            VGA_data.cursor_x = 0;
        break;

        default:                                            // Write character
            VGA_putchr(VGA_data.cursor_x , VGA_data.cursor_y, c);
            VGA_data.cursor_x += 1;
        break;
    }

    if (VGA_data.cursor_x >= VGA_data.width) {                         // Next line when we reach the end of screen
        VGA_data.cursor_y++;
        VGA_data.cursor_x = 0;
    }

    if (VGA_data.cursor_y >= VGA_data.height)
        VGA_scrollback(1);

    VGA_setcursor(VGA_data.cursor_x, VGA_data.cursor_y);
}
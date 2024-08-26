#include "../include/vesa_text.h"


int cursor_x = 0;
int cursor_y = 0;


void VESA_scrollback(int lines) {
    uint32_t bytesPerLine = gfx_mode.x_resolution * (gfx_mode.bits_per_pixel / 8);
    uint32_t screenSize   = gfx_mode.y_resolution * bytesPerLine;
    uint32_t scrollBytes  = lines * bytesPerLine;
    uint8_t* screenBuffer = (uint8_t*)gfx_mode.physical_base_pointer;

    memmove(screenBuffer, screenBuffer + scrollBytes, screenSize - scrollBytes);

    Point fpoint, spoint;
    
    fpoint.X = 0;
    fpoint.Y = gfx_mode.y_resolution - lines;
    spoint.X = gfx_mode.x_resolution;
    spoint.Y = gfx_mode.y_resolution;
    
    GFX_fill_rect_solid(fpoint, spoint, BLACK);
    // GFX_buffer2buffer();
}

void VESA_newline() {
    cursor_x = 0;
    if(cursor_y >= gfx_mode.y_resolution) {
        VESA_scrollback(CHAR_Y);
        cursor_y = gfx_mode.y_resolution - CHAR_Y;
    } 
    else cursor_y += CHAR_Y;
}

void VESA_putc(char c) {
    int _tabSize = 4;
    if (cursor_x + CHAR_X >= gfx_mode.x_resolution) 
        VESA_newline();

    switch (c) {
        case '\n':
            VESA_newline();
            break;

        case '\t':
            for (int i = 0; i < _tabSize - ((gfx_mode.x_resolution) / CHAR_X % _tabSize); i++)
                VESA_putc(' ');
            break;

        default:
            GFX_put_char(cursor_x, cursor_y, c, WHITE, BLACK);
            cursor_x += CHAR_X;
            break;
    }
}

void VESA_cputc(char c, uint32_t fcolor, uint32_t bcolor) {
    int _tabSize = 4;
    if (cursor_x + CHAR_X >= gfx_mode.x_resolution) 
        VESA_newline();

    switch (c) {
        case '\n':
            VESA_newline();
            break;

        case '\t':
            for (int i = 0; i < _tabSize - ((gfx_mode.x_resolution) / CHAR_X % _tabSize); i++)
                VESA_cputc(' ', fcolor, bcolor);
            break;

        default:
            GFX_put_char(cursor_x, cursor_y, c, fcolor, bcolor);
            cursor_x += CHAR_X;
            break;
    }
}

void VESA_memset(uint8_t* dest, uint32_t rgb, uint32_t count) {
    if (count % 3 != 0) count = count + 3 - (count % 3);
    
    uint8_t r = rgb & 0x00ff0000;
    uint8_t g = rgb & 0x0000ff00;
    uint8_t b = rgb & 0x000000ff;
    for (int i = 0; i < count; i++) {
        *dest++ = r;
        *dest++ = g;
        *dest++ = b;
    }
}

void VESA_backspace() {
    if(cursor_x > 0) {
        cursor_x -= CHAR_X;
        VESA_putc(' ');
        cursor_x -= CHAR_X;
    }
}

void VESA_set_cursor(uint8_t x, uint8_t y) {
    cursor_x = x * CHAR_X;
    cursor_y = y * CHAR_Y;
}

void VESA_set_cursor32(uint32_t x, uint32_t y) {
    cursor_x = x;
    cursor_y = y;
}

void VESA_clrscr() {
    Point fpoint, spoint;
    
    fpoint.X = 0;
    fpoint.Y = 0;
    spoint.X = gfx_mode.x_resolution;
    spoint.Y = gfx_mode.y_resolution;
    
    GFX_fill_rect_solid(fpoint, spoint, BLACK);

    cursor_x = 0;
    cursor_y = 0;
}

int VESA_get_cursor_x() {
    return cursor_x;
}

int VESA_get_cursor_y() {
    return cursor_y;
}
#include "../include/vesa.h"


static int _cursor_x = 0;
static int _cursor_y = 0;


void VESA_init() {
    KSTDIO_data.clrscr = VESA_clrscr;
    KSTDIO_data.fill_color = VESA_fill;
    KSTDIO_data.putc = VESA_putc;
    KSTDIO_data.get_cursor_x = VESA_get_cursor_x;
    KSTDIO_data.get_cursor_y = VESA_get_cursor_y;
    KSTDIO_data.set_cursor = VESA_set_cursor;
    KSTDIO_data.put_chr = VESA_putchr;
    KSTDIO_data.get_char = GFX_get_char;
}

void VESA_scrollback(int lines) {
    uint32_t bytesPerLine = gfx_mode.x_resolution * (gfx_mode.bits_per_pixel / 8);
    uint32_t screenSize   = gfx_mode.y_resolution * bytesPerLine;
    uint32_t scrollBytes  = lines * bytesPerLine;
    uint8_t* screenBuffer = (uint8_t*)gfx_mode.physical_base_pointer;

    memmove(screenBuffer, screenBuffer + scrollBytes, screenSize - scrollBytes);

    Point fpoint = {
        .X = 0,
        .Y = VESA_get_max32_y() - lines
    };

    Point spoint = {
        .X = VESA_get_max32_x(),
        .Y = VESA_get_max32_y()
    };
    
    GFX_fill_rect_solid(fpoint, spoint, BLACK);
}

void VESA_newline() {
    _cursor_x = 0;
    if (_cursor_y >= VESA_get_max32_y()) {
        VESA_scrollback(CHAR_Y);
        _cursor_y = VESA_get_max32_y() - CHAR_Y;
    } 
    else _cursor_y += CHAR_Y;
}

void VESA_putchr(uint8_t x, uint8_t y, char c) {
    GFX_put_char(x * CHAR_X, y * CHAR_Y, c, WHITE, BLACK);
}

void VESA_cputchr(uint8_t x, uint8_t y, char c, uint32_t fcolor, uint32_t bcolor) {
    GFX_put_char(x * CHAR_X, y * CHAR_Y, c, fcolor, bcolor);
}

void VESA_putc(char c) {
    VESA_cputc(c, WHITE, BLACK);
}

void VESA_cputc(char c, uint32_t fcolor, uint32_t bcolor) {
    int _tabSize = 4;
    if (_cursor_x + CHAR_X >= VESA_get_max32_x()) 
        VESA_newline();

    switch (c) {
        case '\n':
            VESA_newline();
            break;

        case '\t':
            for (int i = 0; i < _tabSize - ((VESA_get_max32_x()) / CHAR_X % _tabSize); i++)
                VESA_cputc(' ', fcolor, bcolor);
            break;

        default:
            GFX_put_char(_cursor_x, _cursor_y, c, fcolor, bcolor);
            _cursor_x += CHAR_X;
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

void VESA_clrscr() {
    VESA_fill(BLACK);
}

void VESA_fill(uint32_t color) {
    Point fpoint = {
        .X = 0,
        .Y = 0
    };

    Point spoint = {
        .X = VESA_get_max32_x(),
        .Y = VESA_get_max32_y()
    };
    
    GFX_fill_rect_solid(fpoint, spoint, color);

    _cursor_x = 0;
    _cursor_y = 0;
}

void VESA_set_cursor(uint8_t x, uint8_t y) {
    if (x >= 0 && y >= 0) {
        _cursor_x = x * CHAR_X;
        _cursor_y = y * CHAR_Y;
    }
}

void VESA_set_cursor32(uint32_t x, uint32_t y) {
    if (x >= 0 && y >= 0) {
        _cursor_x = x;
        _cursor_y = y;
    }
}

int VESA_get_cursor32_x() {
    return _cursor_x;
}

uint8_t VESA_get_cursor_x() {
    return _cursor_x / CHAR_X;
}

int VESA_get_cursor32_y() {
    return _cursor_y;
}

uint8_t VESA_get_cursor_y() {
    return _cursor_y / CHAR_Y;
}

int VESA_get_max32_x() {
    return gfx_mode.x_resolution;
}

uint8_t VESA_get_max_x() {
    return gfx_mode.x_resolution / CHAR_X;
}

int VESA_get_max32_y() {
    return gfx_mode.y_resolution;
}

uint8_t VESA_get_max_y() {
    return gfx_mode.y_resolution / CHAR_Y;
}
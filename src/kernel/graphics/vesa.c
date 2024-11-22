#include "../include/vesa.h"


static int _cursor_x = 0;
static int _cursor_y = 0;
static uint32_t _chars[CHARCOUNT * CHARLEN];


void VESA_init() {
    KSTDIO_data.clrscr = VESA_clrscr;
    KSTDIO_data.fill_color = VESA_fill;
    KSTDIO_data.putc = VESA_putc;
    KSTDIO_data.get_cursor_x = VESA_get_cursor_x;
    KSTDIO_data.get_cursor_y = VESA_get_cursor_y;
    KSTDIO_data.set_cursor = VESA_set_cursor;
    KSTDIO_data.put_chr = VESA_putchr;
    KSTDIO_data.get_char = __vmem_getc;

    for (uint8_t c = ' '; c < '~'; c++) {
        uint16_t offset = (c - 31) * 16 ;
        for (int row = 0; row < CHAR_Y; row++) {
            uint8_t mask = 1 << 7;
            uint32_t* abs_row = _chars + CHAROFF(c) + (row * 8);
            for (int i = 0; i < 8; i++) {
                if (font.Bitmap[offset + row] & mask) abs_row[i] = CHAR_BODY;
                else abs_row[i] = EMPTY_SPACE;
                mask = (mask >> 1);
            }
        }
    }
}

void VESA_scrollback(int lines) {
    uint32_t bytesPerLine = GFX_data.x_resolution * (GFX_data.bits_per_pixel / 8);
    uint32_t screenSize   = GFX_data.y_resolution * bytesPerLine;
    uint32_t scrollBytes  = lines * bytesPerLine;
    uint8_t* screenBuffer = (uint8_t*)GFX_data.physical_base_pointer;

    memmove(screenBuffer, screenBuffer + scrollBytes, screenSize - scrollBytes);
    __vmem_fill(BLACK, 0, VESA_get_max32_y() - lines, VESA_get_max32_x(), VESA_get_max32_y());
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
    __vmem_putc(x * CHAR_X, y * CHAR_Y, c, WHITE, BLACK);
}

void VESA_cputchr(uint8_t x, uint8_t y, char c, uint32_t fcolor, uint32_t bcolor) {
    __vmem_putc(x * CHAR_X, y * CHAR_Y, c, fcolor, bcolor);
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
            __vmem_putc(_cursor_x, _cursor_y, c, fcolor, bcolor);
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
    __vmem_fill(BLACK, 0, 0, VESA_get_max32_x(), VESA_get_max32_y());
    _cursor_x = 0;
    _cursor_y = 0;
}

void __vmem_fill(uint32_t color, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    for (uint16_t x = a; x < c; x++)
        for (uint16_t y = b; y < d; y++)
            GFX_pdraw_pixel(x, y, color);
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
    return GFX_data.x_resolution;
}

uint8_t VESA_get_max_x() {
    return GFX_data.x_resolution / CHAR_X;
}

int VESA_get_max32_y() {
    return GFX_data.y_resolution;
}

uint8_t VESA_get_max_y() {
    return GFX_data.y_resolution / CHAR_Y;
}

void __vmem_putc(int x, int y, char c, uint32_t foreground, uint32_t background) {
    uint32_t step = GFX_data.pitch / 4;
    uint32_t* chardat = _chars + CHAROFF(c);
    uint32_t* abs_row = (uint32_t*)(((unsigned char*)GFX_data.physical_base_pointer) + (y * GFX_data.pitch));
    abs_row += x;

    for (int row = 0; row < CHAR_Y * 8; row += 8) {
        for (int i = 0; i < 8; i++) {
            uint32_t pixel_color = (chardat[row + i] == CHAR_BODY) ? foreground : background;
            abs_row[i] = pixel_color;
        }
        abs_row += step;
    }
}

char __vmem_getc(int x, int y) {
    uint32_t step = GFX_data.pitch / 4;
    uint32_t* abs_row = (uint32_t*)(((unsigned char*)GFX_data.physical_base_pointer) + (y * GFX_data.pitch));
    abs_row += x;

    uint32_t char_data[32] = { 0 };
    for (int row = 0; row < CHAR_Y * 8; row += 8) {
        memcpy(char_data + row, abs_row, 32);
        abs_row += step;
    }

    char character = 0;
    for (int i = 0; i < CHAR_Y * 8; i++) 
        character |= (char_data[i] & 0x01) << i;

    return character;
}
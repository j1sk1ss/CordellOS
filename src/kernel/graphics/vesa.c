#include "../include/vesa.h"


static int _cursor_x = 0;
static int _cursor_y = 0;
extern uint8_t _binary_src_kernel_font_psf_start;


void VESA_init() {
    KSTDIO_data.clrscr       = VESA_clrscr;
    KSTDIO_data.fill_color   = VESA_fill;
    KSTDIO_data.putc         = VESA_putc;
    KSTDIO_data.get_cursor_x = VESA_get_cursor_x;
    KSTDIO_data.get_cursor_y = VESA_get_cursor_y;
    KSTDIO_data.set_cursor   = VESA_set_cursor;
    KSTDIO_data.put_chr      = VESA_putchr;
    KSTDIO_data.get_char     = __pmem_getc;
}

void VESA_scrollback(int lines) {
    GFX_scrollback_buffer(lines, GFX_data.physical_base_pointer);
    __pmem_fill(BLACK, 0, VESA_get_max32_y() - lines, VESA_get_max32_x(), VESA_get_max32_y());
}

void VESA_newline() {
    _cursor_x = 0;
    if (_cursor_y < VESA_get_max32_y()) _cursor_y += _psf_get_height(&_binary_src_kernel_font_psf_start);
    else {
        VESA_scrollback(_psf_get_height(&_binary_src_kernel_font_psf_start));
        _cursor_y = VESA_get_max32_y() - _psf_get_height(&_binary_src_kernel_font_psf_start);
    }
}

void VESA_putchr(uint8_t x, uint8_t y, char c) {
    __pmem_putc(
        x * _psf_get_width(&_binary_src_kernel_font_psf_start),
        y * _psf_get_height(&_binary_src_kernel_font_psf_start), c, WHITE, BLACK
    );
}

void VESA_cputchr(uint8_t x, uint8_t y, char c, uint32_t fcolor, uint32_t bcolor) {
    __pmem_putc(
        x * _psf_get_width(&_binary_src_kernel_font_psf_start),
        y * _psf_get_height(&_binary_src_kernel_font_psf_start), c, fcolor, bcolor
    );
}

void VESA_putc(char c) {
    VESA_cputc(c, WHITE, BLACK);
}

void VESA_cputc(char c, uint32_t fcolor, uint32_t bcolor) {
    int _tabSize = 4;
    if (_cursor_x + _psf_get_width(&_binary_src_kernel_font_psf_start) >= VESA_get_max32_x()) VESA_newline();
    switch (c) {
        case '\n':
            VESA_newline();
        break;

        case '\t':
            for (int i = 0; i < _tabSize - ((VESA_get_max32_x()) / _psf_get_width(&_binary_src_kernel_font_psf_start) % _tabSize); i++)
                VESA_cputc(' ', fcolor, bcolor);
            break;

        default:
            __pmem_putc(_cursor_x, _cursor_y, c, fcolor, bcolor);
            _cursor_x += _psf_get_width(&_binary_src_kernel_font_psf_start);
        break;
    }
}

void VESA_clrscr() {
    VESA_fill(BLACK);
}

void VESA_fill(uint32_t color) {
    __vmem_fill(BLACK, 0, 0, VESA_get_max32_x(), VESA_get_max32_y());
    GFX_swap_buffers();
    _cursor_x = 0;
    _cursor_y = 0;
}

void __vmem_fill(uint32_t color, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    for (uint16_t x = a; x < c; x++)
        for (uint16_t y = b; y < d; y++)
            GFX_vdraw_pixel(x, y, color);
}

void __pmem_fill(uint32_t color, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    for (uint16_t x = a; x < c; x++)
        for (uint16_t y = b; y < d; y++)
            GFX_pdraw_pixel(x, y, color);
}

void VESA_set_cursor(uint8_t x, uint8_t y) {
    if (x >= 0 && y >= 0 && x < VESA_get_max_x() && y < VESA_get_max_y()) {
        _cursor_x = x * _psf_get_width(&_binary_src_kernel_font_psf_start);
        _cursor_y = y * _psf_get_height(&_binary_src_kernel_font_psf_start);
    }
}

void VESA_set_cursor32(uint32_t x, uint32_t y) {
    if (x >= 0 && y >= 0 && x < VESA_get_max32_x() && y < VESA_get_max32_y()) {
        _cursor_x = x;
        _cursor_y = y;
    }
}

int VESA_get_cursor32_x() {
    return _cursor_x;
}

uint8_t VESA_get_cursor_x() {
    return _cursor_x / _psf_get_width(&_binary_src_kernel_font_psf_start);
}

int VESA_get_cursor32_y() {
    return _cursor_y;
}

uint8_t VESA_get_cursor_y() {
    return _cursor_y / _psf_get_height(&_binary_src_kernel_font_psf_start);
}

int VESA_get_max32_x() {
    return GFX_data.x_resolution;
}

uint8_t VESA_get_max_x() {
    return GFX_data.x_resolution / _psf_get_width(&_binary_src_kernel_font_psf_start);
}

int VESA_get_max32_y() {
    return GFX_data.y_resolution;
}

uint8_t VESA_get_max_y() {
    return GFX_data.y_resolution / _psf_get_height(&_binary_src_kernel_font_psf_start);
}

void __vmem_putc(int x, int y, char c, uint32_t foreground, uint32_t background) {
    __mem_putc(x, y, c, foreground, background, GFX_data.virtual_second_buffer);
}

void __pmem_putc(int x, int y, char c, uint32_t foreground, uint32_t background) {
    __mem_putc(x, y, c, foreground, background, GFX_data.physical_base_pointer);
}

void __mem_putc(int x, int y, char c, uint32_t foreground, uint32_t background, uint32_t buffer) {
    int bytesperline = (_psf_get_width(&_binary_src_kernel_font_psf_start) + 7) / 8;
    uint8_t* glyph = PSF_get_glyph(&_binary_src_kernel_font_psf_start, c);

    /* Calculate the absolute row based on screen coordinates */
    uint32_t step = GFX_data.pitch / sizeof(uint32_t);
    uint32_t* abs_row = (uint32_t*)(((uint8_t*)buffer) + (y * GFX_data.pitch));
    abs_row += x;

    /* Finally display pixels according to the bitmap */
    int j = 0;
    uint32_t mask = 0;
    for (j = 0; j < _psf_get_height(&_binary_src_kernel_font_psf_start); j++) {
        /* Save the starting position of the line */
        mask = 1 << (_psf_get_width(&_binary_src_kernel_font_psf_start) - 1);

        /* Display a row */
        for (int i = 0; i < _psf_get_width(&_binary_src_kernel_font_psf_start); i++) {
            uint32_t pixel_color = (*((uint32_t*)glyph) & mask) ? foreground : background;
            abs_row[i] = pixel_color;

            /* Adjust to the next pixel */
            mask >>= 1;
        }

        /* Adjust to the next line */
        glyph += bytesperline;
        abs_row += step;
    }
}

char __pmem_getc(int x, int y) {
    uint32_t step = GFX_data.pitch / 4;
    uint32_t* abs_row = (uint32_t*)(((uint8_t*)GFX_data.physical_base_pointer) + (y * GFX_data.pitch));
    abs_row += x;

    uint32_t char_data[32] = { 0 };
    for (int row = 0; row < _psf_get_height(&_binary_src_kernel_font_psf_start) * 8; row += 8) {
        memcpy(char_data + row, abs_row, 32);
        abs_row += step;
    }

    char character = 0;
    for (int i = 0; i < _psf_get_height(&_binary_src_kernel_font_psf_start) * 8; i++) 
        character |= (char_data[i] & 0x01) << i;

    return character;
}
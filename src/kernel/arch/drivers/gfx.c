#include "../../include/gfx.h"


vbe_mode_info_t gfx_mode;
uint32_t chars[CHARCOUNT * CHARLEN];


void GFX_init(struct multiboot_info* mb_info) {
    gfx_mode.physical_base_pointer = mb_info->framebuffer_addr;
    gfx_mode.x_resolution          = mb_info->framebuffer_width;
    gfx_mode.y_resolution          = mb_info->framebuffer_height;
    gfx_mode.bits_per_pixel        = mb_info->framebuffer_bpp;
    gfx_mode.pitch                 = mb_info->framebuffer_pitch;

    gfx_mode.linear_red_mask_size      = mb_info->framebuffer_red_mask_size;
    gfx_mode.linear_red_field_position = mb_info->framebuffer_red_field_position;

    gfx_mode.linear_green_mask_size = mb_info->framebuffer_green_mask_size;
    gfx_mode.linear_green_mask_size = mb_info->framebuffer_green_field_position;

    gfx_mode.linear_blue_mask_size      = mb_info->framebuffer_blue_mask_size;
    gfx_mode.linear_blue_field_position = mb_info->framebuffer_blue_field_position;

    gfx_mode.buffer_size = gfx_mode.y_resolution * gfx_mode.x_resolution * (gfx_mode.bits_per_pixel | 7) >> 3;
    gfx_mode.virtual_second_buffer = 0x5000000;
    
    for (unsigned char c = ' '; c < '~'; c++) {
        unsigned short offset = (c - 31) * 16 ;
        for (int row = 0; row < CHAR_Y; row++) {
            uint8_t mask = 1 << 7;
            uint32_t* abs_row = chars + CHAROFF(c) + (row * 8);
            for (int i = 0; i < 8; i++) {
                if(font.Bitmap[offset + row] & mask) abs_row[i] = CHAR_BODY;
                else abs_row[i] = EMPTY_SPACE;
                
                mask = (mask >> 1);
            }
        }
    }

    KSTDIO_data.clrscr = VESA_clrscr;
    KSTDIO_data.fill_color = VESA_fill;
    KSTDIO_data.putc = VESA_putc;
    KSTDIO_data.get_cursor_x = VESA_get_cursor_x;
    KSTDIO_data.get_cursor_y = VESA_get_cursor_y;
    KSTDIO_data.set_cursor = VESA_set_cursor;
    KSTDIO_data.put_chr = VESA_putchr;
    KSTDIO_data.get_char = GFX_get_char;
}

void GFX_draw_pixel(uint16_t X, uint16_t Y, uint32_t color) {
    if (color == TRANSPARENT) return;
    uint8_t* framebuffer    = (uint8_t*)gfx_mode.virtual_second_buffer; 
    uint8_t bytes_per_pixel = (gfx_mode.bits_per_pixel + 1) / 8;

    framebuffer += (Y * gfx_mode.x_resolution + X) * bytes_per_pixel;
    for (uint8_t temp = 0; temp < bytes_per_pixel; temp++)
        framebuffer[temp] = (uint8_t)(color >> temp * 8);
}

void GFX_vdraw_pixel(uint16_t X, uint16_t Y, uint32_t color) {
    if (color == TRANSPARENT) return;
    uint8_t* framebuffer    = (uint8_t*)gfx_mode.physical_base_pointer; 
    uint8_t bytes_per_pixel = (gfx_mode.bits_per_pixel + 1) / 8;

    framebuffer += (Y * gfx_mode.x_resolution + X) * bytes_per_pixel;
    for (uint8_t temp = 0; temp < bytes_per_pixel; temp++)
        framebuffer[temp] = (uint8_t)(color >> temp * 8);
}

uint32_t GFX_get_pixel(uint16_t X, uint16_t Y) {
    uint8_t* framebuffer = (uint8_t*)gfx_mode.physical_base_pointer;
    uint8_t bytes_per_pixel = (gfx_mode.bits_per_pixel + 1) / 8;

    framebuffer += (Y * gfx_mode.x_resolution + X) * bytes_per_pixel;
    uint32_t color = 0;

    for (uint8_t temp = 0; temp < bytes_per_pixel; temp++) 
        color |= ((uint32_t)framebuffer[temp] << temp * 8);

    return color;
}

void GFX_fill_rect_solid(Point top_left, Point bottom_right, uint32_t color) {
   for (uint16_t y = top_left.Y; y < bottom_right.Y; y++)
       for (uint16_t x = top_left.X; x < bottom_right.X; x++)
           GFX_vdraw_pixel(x, y, color);
}

void GFX_put_char(int x, int y, int character, uint32_t foreground, uint32_t background) {
    uint32_t step = gfx_mode.pitch / 4;
    uint32_t* chardat = chars + CHAROFF(character);
    uint32_t* abs_row = (uint32_t*)(((unsigned char*)gfx_mode.physical_base_pointer) + (y * gfx_mode.pitch));
    abs_row += x;

    for (int row = 0; row < CHAR_Y * 8; row += 8) {
        for (int i = 0; i < 8; i++) {
            uint32_t pixel_color = (chardat[row + i] == CHAR_BODY) ? foreground : background;
            abs_row[i] = pixel_color;
        }
        abs_row += step;
    }
}


int GFX_get_char(int x, int y) {
    uint32_t step = gfx_mode.pitch / 4;
    uint32_t* abs_row = (uint32_t*)(((unsigned char*)gfx_mode.physical_base_pointer) + (y * gfx_mode.pitch));
    abs_row += x;

    uint32_t char_data[32] = { 0 };
    for (int row = 0; row < CHAR_Y * 8; row += 8) {
        memcpy(char_data + row, abs_row, 32);
        abs_row += step;
    }

    int character = 0;
    for (int i = 0; i < CHAR_Y * 8; i++) 
        character |= (char_data[i] & 0x01) << i;

    return character;
}

uint32_t GFX_convert_color(const uint32_t color) {
    uint8_t convert_r, convert_g, convert_b;
    uint32_t converted_color = 0;

    const uint8_t orig_r = (color >> 16) & 0xFF;
    const uint8_t orig_g = (color >> 8)  & 0xFF;
    const uint8_t orig_b = color         & 0xFF;

    if (gfx_mode.bits_per_pixel == 8) {
        convert_r = 0;
        convert_g = 0;
        convert_b = orig_b;
    } else {
        const uint8_t r_bits_to_shift = 8 - gfx_mode.linear_red_mask_size; 
        const uint8_t g_bits_to_shift = 8 - gfx_mode.linear_green_mask_size;
        const uint8_t b_bits_to_shift = 8 - gfx_mode.linear_blue_mask_size;

        convert_r = (orig_r >> r_bits_to_shift) & ((1 << gfx_mode.linear_red_mask_size) - 1);
        convert_g = (orig_g >> g_bits_to_shift) & ((1 << gfx_mode.linear_green_mask_size) - 1);
        convert_b = (orig_b >> b_bits_to_shift) & ((1 << gfx_mode.linear_blue_mask_size) - 1);
    }

    converted_color = (convert_r << gfx_mode.linear_red_field_position)   |
                      (convert_g << gfx_mode.linear_green_field_position) |
                      (convert_b << gfx_mode.linear_blue_field_position);

    return converted_color;
}

void GFX_buffer2buffer() {
    uint8_t* framebuffer = (uint8_t*)gfx_mode.physical_base_pointer;
    uint8_t* back_framebuffer = (uint8_t*)gfx_mode.virtual_second_buffer;
    memcpy32(framebuffer, back_framebuffer, gfx_mode.buffer_size);
}
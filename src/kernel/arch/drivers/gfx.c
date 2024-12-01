#include "../../include/gfx.h"


vbe_mode_info_t GFX_data;


void GFX_init(struct multiboot_info* mb_info) {
    GFX_data.physical_base_pointer = mb_info->framebuffer_addr;
    GFX_data.x_resolution          = mb_info->framebuffer_width;
    GFX_data.y_resolution          = mb_info->framebuffer_height;
    GFX_data.bits_per_pixel        = mb_info->framebuffer_bpp;
    GFX_data.pitch                 = mb_info->framebuffer_pitch;

    GFX_data.linear_red_mask_size      = mb_info->framebuffer_red_mask_size;
    GFX_data.linear_red_field_position = mb_info->framebuffer_red_field_position;

    GFX_data.linear_green_mask_size = mb_info->framebuffer_green_mask_size;
    GFX_data.linear_green_mask_size = mb_info->framebuffer_green_field_position;

    GFX_data.linear_blue_mask_size      = mb_info->framebuffer_blue_mask_size;
    GFX_data.linear_blue_field_position = mb_info->framebuffer_blue_field_position;

    GFX_data.buffer_size = GFX_data.y_resolution * GFX_data.x_resolution * (GFX_data.bits_per_pixel | 7) >> 3;
    GFX_data.virtual_second_buffer = GFX_data.physical_base_pointer + GFX_data.buffer_size;
}

void GFX_vdraw_pixel(uint16_t X, uint16_t Y, uint32_t color) {
    if (color == TRANSPARENT) return;
    __draw_pixel(X, Y, color, GFX_data.virtual_second_buffer);
}

void GFX_pdraw_pixel(uint16_t X, uint16_t Y, uint32_t color) {
    if (color == TRANSPARENT) return;
    __draw_pixel(X, Y, color, GFX_data.physical_base_pointer);
}

void __draw_pixel(uint16_t x, uint16_t y, uint32_t color, uint32_t buffer) {
    uint8_t* framebuffer    = (uint8_t*)buffer; 
    uint8_t bytes_per_pixel = (GFX_data.bits_per_pixel + 1) / 8;

    framebuffer += (y * GFX_data.x_resolution + x) * bytes_per_pixel;
    for (uint8_t temp = 0; temp < bytes_per_pixel; temp++)
        framebuffer[temp] = (uint8_t)(color >> temp * 8);
}

uint32_t GFX_get_pixel(uint16_t X, uint16_t Y) {
    uint8_t* framebuffer = (uint8_t*)GFX_data.physical_base_pointer;
    uint8_t bytes_per_pixel = (GFX_data.bits_per_pixel + 1) / 8;

    framebuffer += (Y * GFX_data.x_resolution + X) * bytes_per_pixel;
    uint32_t color = 0;

    for (uint8_t temp = 0; temp < bytes_per_pixel; temp++) 
        color |= ((uint32_t)framebuffer[temp] << temp * 8);

    return color;
}

uint32_t GFX_convert_color(const uint32_t color) {
    uint8_t convert_r = 0, convert_g = 0, convert_b = 0;
    uint32_t converted_color = 0;

    const uint8_t orig_r = (color >> 16) & 0xFF;
    const uint8_t orig_g = (color >> 8)  & 0xFF;
    const uint8_t orig_b = color         & 0xFF;

    if (GFX_data.bits_per_pixel == 8) {
        convert_r = 0;
        convert_g = 0;
        convert_b = orig_b;
    } else {
        const uint8_t r_bits_to_shift = 8 - GFX_data.linear_red_mask_size; 
        const uint8_t g_bits_to_shift = 8 - GFX_data.linear_green_mask_size;
        const uint8_t b_bits_to_shift = 8 - GFX_data.linear_blue_mask_size;

        convert_r = (orig_r >> r_bits_to_shift) & ((1 << GFX_data.linear_red_mask_size) - 1);
        convert_g = (orig_g >> g_bits_to_shift) & ((1 << GFX_data.linear_green_mask_size) - 1);
        convert_b = (orig_b >> b_bits_to_shift) & ((1 << GFX_data.linear_blue_mask_size) - 1);
    }

    converted_color = (convert_r << GFX_data.linear_red_field_position)   |
                      (convert_g << GFX_data.linear_green_field_position) |
                      (convert_b << GFX_data.linear_blue_field_position);

    return converted_color;
}

void GFX_scrollback_buffer(int lines, uint32_t buffer) {
    uint32_t bytesPerLine = GFX_data.x_resolution * (GFX_data.bits_per_pixel / 8);
    uint32_t screenSize   = GFX_data.y_resolution * bytesPerLine;
    uint32_t scrollBytes  = lines * bytesPerLine;
    uint8_t* screenBuffer = (uint8_t*)buffer;
    memmove(screenBuffer, screenBuffer + scrollBytes, screenSize - scrollBytes);
}

void GFX_swap_buffers() {
    memcpy((void*)GFX_data.physical_base_pointer, (void*)GFX_data.virtual_second_buffer, GFX_data.buffer_size);
}

void GFX_set_pbuffer(uint32_t value) {
    __set_buffer(value, GFX_data.physical_base_pointer, GFX_data.buffer_size);
}

void GFX_set_vbuffer(uint32_t value) {
    __set_buffer(value, GFX_data.virtual_second_buffer, GFX_data.buffer_size);
}

void __set_buffer(uint32_t value, uint32_t addr, size_t size) {
    for (size_t i = 0; i < size; i++) ((uint32_t*)addr)[i] = value;
}
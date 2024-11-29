#ifndef GFX_H_
#define GFX_H_

#include <stdint.h>
#include <math.h>

#include "allocator.h"
#include "font.h"
#include "x86.h"

#include "../multiboot/multiboot.h"


#define BLACK       0x00000000 
#define WHITE       0x00FFFFFF 
#define DARK_GRAY   0x00222222
#define LIGHT_GRAY  0x00DDDDDD
#define RED         0x00FF0000 
#define GREEN       0x0000FF00 
#define BLUE        0x000000FF 
#define YELLOW      0x00FFFF00 
#define PURPLE      0x00FF00FF
#define TRANSPARENT 0x10000000

#define ROUND(a) ((int)(a + 0.5))


typedef struct {
	
	uint16_t mode_attributes;
	uint8_t window_a_attributes;
	uint8_t window_b_attributes;
	uint16_t window_granularity;
	uint16_t window_size;
	uint16_t window_a_segment;
	uint16_t window_b_segment;
	uint32_t window_function_pointer;
	uint16_t bytes_per_scanline;

	uint16_t x_resolution;
	uint16_t y_resolution;
	uint32_t pitch;

	uint8_t x_charsize;
	uint8_t y_charsize;
	uint8_t number_of_planes;
	uint8_t bits_per_pixel;
	uint8_t number_of_banks;
	uint8_t memory_model;
	uint8_t bank_size;
	uint8_t number_of_image_pages;
	uint8_t reserved1;

	uint8_t red_mask_size;
	uint8_t red_field_position;
	uint8_t green_mask_size;
	uint8_t green_field_position;
	uint8_t blue_mask_size;
	uint8_t blue_field_position;
	uint8_t reserved_mask_size;
	uint8_t reserved_field_position;
	uint8_t direct_color_mode_info;

	uint32_t physical_base_pointer;
	uint32_t virtual_second_buffer;
	uint32_t buffer_size;
	uint32_t reserved2;
	uint16_t reserved3;

	uint16_t linear_bytes_per_scanline;
    uint8_t bank_number_of_image_pages;
    uint8_t linear_number_of_image_pages;
    uint8_t linear_red_mask_size;
    uint8_t linear_red_field_position;
    uint8_t linear_green_mask_size;
    uint8_t linear_green_field_position;
    uint8_t linear_blue_mask_size;
    uint8_t linear_blue_field_position;
    uint8_t linear_reserved_mask_size;
    uint8_t linear_reserved_field_position;
    uint32_t max_pixel_clock;

    uint8_t reserved4[190];
} __attribute__ ((packed)) vbe_mode_info_t;

typedef struct {
	
    char signature[4];
    uint16_t version;
    uint32_t oem_string_ptr;
    uint32_t capabilities;
    uint32_t video_mode_ptr;
    uint16_t total_memory;
    uint16_t oem_software_rev;
    uint32_t oem_vendor_name_ptr;
    uint32_t oem_product_name_ptr;
    uint32_t oem_product_rev_ptr;
    uint8_t reserved[222];
    uint8_t oem_data[256];
} __attribute__((packed)) vbe_controller_info_t;

typedef struct {
    uint32_t fg_color;
    uint32_t bg_color;
} user_gfx_info_t;

typedef struct point {
    uint16_t X, Y;
} Point;


extern vbe_mode_info_t GFX_data;


void GFX_init(struct multiboot_info* mb_info);

uint32_t GFX_get_pixel(uint16_t X, uint16_t Y);
void GFX_vdraw_pixel(uint16_t X, uint16_t Y, uint32_t color);
void GFX_pdraw_pixel(uint16_t X, uint16_t Y, uint32_t color);
void __draw_pixel(uint16_t x, uint16_t y, uint32_t color, uint32_t buffer);

uint32_t GFX_convert_color(const uint32_t color);
void GFX_swap_buffers();
void GFX_set_pbuffer(uint32_t value);
void GFX_set_vbuffer(uint32_t value);
void __set_buffer(uint32_t value, uint32_t addr, size_t size);

#endif
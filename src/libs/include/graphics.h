#ifndef GRAPHICS_H_
#define GRAPHICS_H_


#include <stdint.h>

#include "bitmap.h"
#include "psf.h"
#include "stdlib.h"
#include "fslib.h"


#define GET_ALPHA(color)        ((color >> 24) & 0x000000FF)
#define GET_RED(color)          ((color >> 16) & 0x000000FF)
#define GET_GREEN(color)        ((color >> 8) & 0x000000FF)
#define GET_BLUE(color)         ((color >> 0) & 0X000000FF)
#define SET_ALPHA(color, alpha) (((color << 8) >> 8) | ((alpha << 24) & 0xFF000000))

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


typedef struct GUIobject {
    int x, y;
    int prev_x, prev_y;
    int height, width;

    uint32_t background_color;
    
    int children_count;
    struct GUIobject** childrens;

    int bitmap_count;
    struct bitmap** bitmaps;

    int text_count;
    struct text_object** texts;
} GUIobject_t;

typedef struct text_object {
    uint32_t x, y;
    char* text;
    uint32_t char_count;
    uint32_t bg_color;
} text_object_t;


void swipe_buffers();

void load_font(char* path);
void unload_font();
void display_char(int x, int y, char c, uint32_t foreground, uint32_t background);

int get_resolution_x();
int get_resolution_y();

GUIobject_t* create_gui_object(int x, int y, int height, int width, uint32_t background);
GUIobject_t* object_add_children(GUIobject_t* object, GUIobject_t* children);
GUIobject_t* object_add_bitmap(GUIobject_t* object, struct bitmap* bmp);
GUIobject_t* object_add_text(GUIobject_t* object, text_object_t* text);

text_object_t* create_text(int x, int y, char* text, uint32_t background_color);
void put_text(text_object_t* text);
void unload_text(text_object_t* text);

void object_move(GUIobject_t* object, int rel_x, int rel_y);

void display_gui_object(GUIobject_t* object);
void unload_gui_object(GUIobject_t* object);

void pput_pixel(int x, int y, int color);
void vput_pixel(int x, int y, int color);
int get_pixel(int x, int y);

uint32_t blend_colors(uint32_t first_color, uint32_t second_color);

#endif
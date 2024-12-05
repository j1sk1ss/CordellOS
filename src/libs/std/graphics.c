#include "../include/graphics.h"


//====================================================================
// Function put pixel by coordinates to virtual memory (slow method)
// EBX - x
// ECX - y
// EDX - pixel data
void pput_pixel(int x, int y, int color) {
    __asm__ volatile(
        "movl $28, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "movl %2, %%edx\n"
        "int $0x80\n"
        :
        : "r"(x), "r"(y), "r"(color)
        : "eax", "ebx", "ecx"
    );
}

//====================================================================
// Function directly in virtual second buffer memory put pixel by coordinates
// EBX - x
// ECX - y
// EDX - pixel data
void vput_pixel(int x, int y, int color) {
    __asm__ volatile(
        "movl $37, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "movl %2, %%edx\n"
        "int $0x80\n"
        :
        : "r"(x), "r"(y), "r"(color)
        : "eax", "ebx", "ecx"
    );
}

//====================================================================
// Function get pixel from framebuffer by coordinates
// EBX - x
// ECX - y
// EDX - result
int get_pixel(int x, int y) {
    int result = 0;
    __asm__ volatile(
        "movl $29, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "movl %2, %%edx\n"
        "int $0x80\n"
        :
        : "r"(x), "r"(y), "r"(result)
        : "eax", "ebx", "ecx"
    );

    return result;
}

//====================================================================
// Function get pixel from framebuffer by coordinates
// EDX - result
int get_resolution_x() {
    int result = 0;
    __asm__ volatile(
        "movl $31, %%eax\n"
        "movl %0, %%edx\n"
        "int $0x80\n"
        :
        : "r"(&result)
        : "eax", "edx"
    );

    return result;
}

//====================================================================
// Function get pixel from framebuffer by coordinates
// EDX - result
int get_resolution_y() {
    int result = 0;
    __asm__ volatile(
        "movl $32, %%eax\n"
        "movl %0, %%edx\n"
        "int $0x80\n"
        :
        : "r"(&result)
        : "eax", "edx"
    );

    return result;
}

//====================================================================
// Function swipe video buffer with second buffer
void swipe_buffers() {
    __asm__ volatile(
        "movl $36, %%eax\n"
        "int $0x80\n"
        :
        :
        : "eax"
    );
}

//====================================================================
// Function scroll screen buffer by lines of pixels
// EBX - lines
void scroll(int lines) {
    __asm__ volatile(
        "movl $47, %%eax\n"
        "movl %0, %%ebx\n"
        "int $0x80\n"
        :
        : "r"(lines)
        : "eax", "ebx"
    );
}

static uint8_t* _cur_font = NULL;

void load_font(char* path) {
    int font_file = copen(path);
    CInfo_t content_info;
    cstat(font_file, &content_info);

    _cur_font = (uint8_t*)malloc(content_info.size);
    fread(font_file, 0, _cur_font, content_info.size);
    cursor_set32(0, 0);
}

uint8_t* get_font() {
    return _cur_font;
}

void unload_font() {
    if (_cur_font == NULL) return;
    free(_cur_font);
    _cur_font = NULL;
}

void display_str(int x, int y, char* str, uint32_t foreground, uint32_t background) {
    int curr_x = x;
    int char_w = _psf_get_width(get_font());
    while (*str) {
        display_char(curr_x, y, *str, foreground, background);
        curr_x += char_w;
        str++;
    }
}

void display_char(int x, int y, char c, uint32_t foreground, uint32_t background) {
    int char_w = _psf_get_width(get_font());
    int char_h = _psf_get_height(get_font());

    int bytesperline = (char_w + 7) / 8;
    uint8_t* glyph = PSF_get_glyph(_cur_font, c);

    /* Finally display pixels according to the bitmap */
    uint32_t mask = 0;
    for (int j = 0; j < char_h; j++) {
        /* Save the starting position of the line */
        mask = 1 << (char_w - 1);

        /* Display a row */
        for (int i = 0; i < char_w; i++) {
            uint32_t pixel_color = (*((uint32_t*)glyph) & mask) ? foreground : background;
            pput_pixel(x + i, y + j, pixel_color);
            mask >>= 1;
        }

        /* Adjust to the next line */
        glyph += bytesperline;
    }
}

void display_gui_object(GUIobject_t* object) {
    if (object == NULL) return;
    for (int y = object->height - 1; y >= 0; y--)
        for (int x = 0; x < object->width; x++)
            vput_pixel(x + object->x, y + object->y, object->background_color);

    for (int i = 0; i < object->children_count; i++) display_gui_object(object->childrens[i]);
    for (int i = 0; i < object->bitmap_count; i++) BMP_display(object->bitmaps[i]);
    for (int i = 0; i < object->text_count; i++) put_text(object->texts[i]);

    swipe_buffers();
}

GUIobject_t* create_gui_object(int x, int y, int height, int width, uint32_t background) {
    GUIobject_t* newObject = (GUIobject_t*)malloc(sizeof(GUIobject_t));
    if (newObject != NULL) {
        newObject->x      = x;
        newObject->y      = y;
        newObject->prev_x = x;
        newObject->prev_y = y;

        newObject->height = height;
        newObject->width  = width;

        newObject->background_color = background;

        newObject->children_count = 0;
        newObject->childrens      = NULL;

        newObject->bitmap_count = 0;
        newObject->bitmaps      = NULL;

        newObject->text_count = 0;
        newObject->texts      = NULL;
    }

    return newObject;
}

void object_move(GUIobject_t* object, int rel_x, int rel_y) {
    if (object == NULL) return;
    for (int i = 0; i < object->children_count; i++)
        object_move(object->childrens[i], rel_x, rel_y);

    for (int i = 0; i < object->bitmap_count; i++) {
        object->bitmaps[i]->x += rel_x;
        object->bitmaps[i]->y += rel_y;
    }

    for (int i = 0; i > object->text_count; i++) {
        object->texts[i]->x += rel_x;
        object->texts[i]->y += rel_y;
    }

    object->x += rel_x;
    object->y += rel_y;
}

GUIobject_t* object_add_children(GUIobject_t* object, GUIobject_t* children) {
    if (object != NULL && children != NULL) {
        object->childrens = (GUIobject_t**)realloc(object->childrens, (object->children_count + 1) * sizeof(GUIobject_t*));
        if (object->childrens != NULL) object->childrens[object->children_count++] = children;
    }

    return object;
}

GUIobject_t* object_add_bitmap(GUIobject_t* object, bitmap_t* bmp) {
    if (object != NULL && bmp != NULL) {
        object->bitmaps = (bitmap_t**)realloc(object->bitmaps, (object->bitmap_count + 1) * sizeof(bitmap_t*));
        if (object->bitmaps != NULL) object->bitmaps[object->bitmap_count++] = bmp;
    }

    return object;
}

GUIobject_t* object_add_text(GUIobject_t* object, text_object_t* text) {
    if (object != NULL && text != NULL) {
        object->texts = (text_object_t**)realloc(object->texts, (object->text_count + 1) * sizeof(text_object_t*));
        if (object->texts != NULL) object->texts[object->text_count++] = text;
    }

    return object;
}

void unload_gui_object(GUIobject_t* object) {
    if (object == NULL) return;

    for (int i = 0; i < object->children_count; i++) unload_gui_object(object->childrens[i]);
    for (int i = 0; i < object->bitmap_count; i++) BMP_unload(object->bitmaps[i]);
    for (int i = 0; i < object->text_count; i++) unload_text(object->texts[i]);

    free(object->childrens);
    free(object->bitmaps);
    free(object->texts);
    free(object);
}

text_object_t* create_text(int x, int y, char* text, uint32_t fcolor, uint32_t bcolor) {
    text_object_t* object = malloc(sizeof(text_object_t));

    object->x = x;
    object->y = y;
    object->char_count = strlen(text);
    object->text       = (char*)malloc(object->char_count + 1);
    object->fg_color   = fcolor;
    object->bg_color   = bcolor;
    strncpy(object->text, text, object->char_count);

    return object;
}

void put_text(text_object_t* text) {
    if (text->x > get_resolution_x() || text->y > get_resolution_y() ||
        text->x < 0 || text->y < 0) {
        printf("\nWrong resolution");
        return;
    }

    display_str(text->x, text->y, text->text, text->fg_color, text->bg_color);
}

void unload_text(text_object_t* text)  {
    free(text->text);
    free(text);
}

uint32_t blend_colors(uint32_t first_color, uint32_t second_color) {
    uint32_t first_alpha = GET_ALPHA(first_color);
    uint32_t first_red   = GET_RED(first_color);
    uint32_t first_green = GET_GREEN(first_color);
    uint32_t first_blue  = GET_BLUE(first_color);

    uint32_t second_alpha = GET_ALPHA(second_color);
    uint32_t second_red   = GET_RED(second_color);
    uint32_t second_green = GET_GREEN(second_color);
    uint32_t second_blue  = GET_BLUE(second_color);

    uint32_t r = (uint32_t)((first_alpha * 1.0 / 255) * first_red);
    uint32_t g = (uint32_t)((first_alpha * 1.0 / 255) * first_green);
    uint32_t b = (uint32_t)((first_alpha * 1.0 / 255) * first_blue);

    r = r + (((255 - first_alpha) * 1.0 / 255) * (second_alpha * 1.0 / 255)) * second_red;
    g = g + (((255 - first_alpha) * 1.0 / 255) * (second_alpha * 1.0 / 255)) * second_green;
    b = b + (((255 - first_alpha) * 1.0 / 255) * (second_alpha * 1.0 / 255)) * second_blue;

    uint32_t new_alpha = (uint32_t)(first_alpha + ((255 - first_alpha) * 1.0 / 255) * second_alpha);
    uint32_t color1_over_color2 = (new_alpha << 24) |  (r << 16) | (g << 8) | (b << 0);
    return color1_over_color2;
}
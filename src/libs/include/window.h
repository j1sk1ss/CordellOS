#ifndef WINDOW_H_
#define WINDOW_H_

#include <stdint.h>

#include "stdlib.h"
#include "graphics.h"

typedef struct window {
    char*             name;
    int               x, y;
    int               height, width;
    uint32_t          background_color;
    int               is_active;
    struct GUIobject* objects;
    int               pid;
    struct window*   next;
} window_t;

window_t* create_window(char* name, int pid, int x, int y, int height, int width, uint32_t bg_color, GUIobject_t* objects);
void      add_window(window_t* window);
void      display_window(window_t* window);
void      unload_window(window_t* window);
window_t* add_text2window(window_t* window, text_object_t* text);
window_t* add_object2window(window_t* window, GUIobject_t* object);

#endif
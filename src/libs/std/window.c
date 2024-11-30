#include "../include/window.h"


window_t* windows = NULL;


window_t* create_window(char* name, int pid, int x, int y, int height, int width, uint32_t bg_color, GUIobject_t* objects) {
    window_t* window = malloc(sizeof(window_t));

    window->name    = name;
    window->next    = NULL;
    window->x       = x;
    window->y       = y;
    window->height  = height;
    window->width   = width;
    window->objects = objects;
    window->pid     = pid;
    window->background_color = bg_color;

    if (objects != NULL) object_move(window->objects, window->x, window->y);
    return window;
}

void add_window(window_t* window) {
    if (windows == NULL) windows = window;
    else {
        window_t* cur = windows;
        while (cur->next != NULL) cur = cur->next;
        cur->next = window;
    }
}

void display_window(window_t* window) {
    GUIobject_t* window_image   = create_gui_object(window->x, window->y, window->height, window->width, window->background_color);
    GUIobject_t* title_image    = create_gui_object(window->x, window->y, 25, window->width, BLUE);
    text_object_t* window_title = create_text(window->x + 10, window->y + 5, window->name, WHITE, BLUE);

    window_image = object_add_children(window_image, title_image);
    window_image = object_add_text(window_image, window_title);

    display_gui_object(window_image);
    if (window->objects != NULL) display_gui_object(window->objects);

    unload_gui_object(window_image);
}

void unload_window(window_t* window) {
    if (window->objects != NULL) unload_gui_object(window->objects);
    free(window);
}

window_t* add_text2window(window_t* window, text_object_t* text) {
    window->objects = object_add_text(window->objects, text);
    return window;
}

window_t* add_object2window(window_t* window, GUIobject_t* object) {
    window->objects = object_add_children(window->objects, object);
    return window;
}
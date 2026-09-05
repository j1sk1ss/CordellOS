#include "doomkeys.h"
#include "doomgeneric.h"

#include <graphics.h>
#include <keyboard.h>
#include <stdlib.h>
#include <time.h>

#define KEYQUEUE_SIZE 32

static unsigned short key_queue[KEYQUEUE_SIZE];
static unsigned int key_queue_write;
static unsigned int key_queue_read;
static int screen_x;
static int screen_y;

static unsigned char convert_to_doom_key(char key)
{
    if (key == ENTER_BUTTON) {
        return KEY_ENTER;
    }
    if (key == STOP_KEYBOARD || key == 27) {
        return KEY_ESCAPE;
    }
    if (key == LEFT_ARROW_BUTTON) {
        return KEY_LEFTARROW;
    }
    if (key == RIGHT_ARROW_BUTTON) {
        return KEY_RIGHTARROW;
    }
    if (key == UP_ARROW_BUTTON) {
        return KEY_UPARROW;
    }
    if (key == DOWN_ARROW_BUTTON) {
        return KEY_DOWNARROW;
    }
    if (key == ' ') {
        return KEY_USE;
    }
    if (key == 'z' || key == 'Z') {
        return KEY_FIRE;
    }
    if (key == LSHIFT_BUTTON || key == RSHIFT_BUTTON) {
        return KEY_RSHIFT;
    }

    return (unsigned char)key;
}

static void add_key(int pressed, unsigned char key)
{
    if (key == 0 || key == '\15') {
        return;
    }

    key_queue[key_queue_write] = (pressed << 8) | key;
    key_queue_write = (key_queue_write + 1) % KEYQUEUE_SIZE;
}

static void poll_keyboard(void)
{
    char key = get_char();
    unsigned char doom_key = convert_to_doom_key(key);

    if (doom_key != 0 && key != '\15') {
        add_key(1, doom_key);
        add_key(0, doom_key);
    }
}

void DG_Init(void)
{
    load_font("home\\shell.psf");

    int xres = get_resolution_x();
    int yres = get_resolution_y();

    screen_x = 0;
    screen_y = 0;
    if (xres > DOOMGENERIC_RESX) {
        screen_x = (xres - DOOMGENERIC_RESX) / 2;
    }
    if (yres > DOOMGENERIC_RESY) {
        screen_y = (yres - DOOMGENERIC_RESY) / 2;
    }

    clrscr();
}

void DG_DrawFrame(void)
{
    for (int y = 0; y < DOOMGENERIC_RESY; y++) {
        for (int x = 0; x < DOOMGENERIC_RESX; x++) {
            vput_pixel(screen_x + x, screen_y + y, DG_ScreenBuffer[y * DOOMGENERIC_RESX + x]);
        }
    }

    swipe_buffers();
    poll_keyboard();
}

void DG_SleepMs(uint32_t ms)
{
    sleep_ms(ms);
}

uint32_t DG_GetTicksMs(void)
{
    return (uint32_t)get_tick();
}

int DG_GetKey(int* pressed, unsigned char* doom_key)
{
    if (key_queue_read == key_queue_write) {
        return 0;
    }

    unsigned short data = key_queue[key_queue_read];
    key_queue_read = (key_queue_read + 1) % KEYQUEUE_SIZE;

    *pressed = data >> 8;
    *doom_key = data & 0xff;
    return 1;
}

void DG_SetWindowTitle(const char* title)
{
    (void)title;
}

int main(int argc, char** argv)
{
    char* normalized_argv[16];

    if (argc > 0 && argv != NULL && argv[0] != NULL && argv[0][0] == '-') {
        int normalized_argc = argc + 1;
        if (normalized_argc > 16) {
            normalized_argc = 16;
        }

        normalized_argv[0] = "doom.elf";
        for (int i = 1; i < normalized_argc; i++) {
            normalized_argv[i] = argv[i - 1];
        }

        argc = normalized_argc;
        argv = normalized_argv;
    }

    doomgeneric_Create(argc, argv);

    while (1) {
        doomgeneric_Tick();
    }

    return 0;
}

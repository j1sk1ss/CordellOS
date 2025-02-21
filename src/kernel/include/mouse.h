#ifndef MOUSE_H_
#define MOUSE_H_

#include "x86.h"
#include "irq.h"
#include "gfx.h"
#include "kstdio.h"


#define MOUSE_IRQ   12
#define MOUSE_XSIZE 5
#define MOUSE_YSIZE 5

#define MOUSE_PORT   0x60
#define MOUSE_STATUS 0x64
#define MOUSE_ABIT   0x02
#define MOUSE_BBIT   0x01
#define MOUSE_WRITE  0xD4
#define MOUSE_F_BIT  0x20
#define MOUSE_V_BIT  0x08

#define LEFT_BUTTON(flag)   (flag & 0x1)
#define RIGHT_BUTTON(flag)  (flag & 0x2)
#define MIDDLE_BUTTON(flag) (flag & 0x4)


typedef struct mouse_state {
    int32_t x;
    int32_t y;
    uint8_t leftButton;
    uint8_t middleButton;
    uint8_t rightButton;
} mouse_state_t;

typedef struct screen_state {
    int32_t x;
    int32_t y;
    uint32_t buffer[MOUSE_XSIZE * MOUSE_YSIZE];
} screen_state_t;


int i386_init_mouse(int show_mouse);
void i386_mouse_handler(struct Registers* regs);
int i386_detect_ps2_mouse();

#endif
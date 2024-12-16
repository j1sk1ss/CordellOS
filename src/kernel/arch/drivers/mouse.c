#include "../../include/mouse.h" 


static int _is_mouse_showing = 0;
static mouse_state_t mouse_state;
static screen_state_t screen_state;
static uint32_t __cursor_bitmap__[] = {
    WHITE, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT,
    WHITE, WHITE, TRANSPARENT, TRANSPARENT, TRANSPARENT,
    WHITE, WHITE, WHITE, TRANSPARENT, TRANSPARENT,
    WHITE, WHITE, WHITE, WHITE, TRANSPARENT,
    TRANSPARENT, WHITE, WHITE, TRANSPARENT, TRANSPARENT,
};


void __mouse_wait(uint8_t a_type) {
	uint32_t timeout = 100000;
	if (!a_type) { 
        while (--timeout) { 
            if ((i386_inb(MOUSE_STATUS) & MOUSE_BBIT) == 1) 
                return; 
        } 
    }
    else { 
        while (--timeout) { 
            if (!((i386_inb(MOUSE_STATUS) & MOUSE_ABIT))) return; 
        } 
    }
}

void __mouse_write(uint8_t write) {
	__mouse_wait(1);
	i386_outb(MOUSE_STATUS, MOUSE_WRITE);
	__mouse_wait(1);
	i386_outb(MOUSE_PORT, write);
}

uint8_t __mouse_read() {
	__mouse_wait(0);
	return i386_inb(MOUSE_PORT);
}

void __place_cursor() {
    if (screen_state.x < 0 || screen_state.y < 0) {
        screen_state.x = mouse_state.x;
        screen_state.y = mouse_state.y;
    }

    if (screen_state.x == mouse_state.x && screen_state.y == mouse_state.y) return;
    if (screen_state.x != -1 && screen_state.y != -1) 
        for (uint16_t i = screen_state.x; i < min(GFX_data.x_resolution, screen_state.x + MOUSE_XSIZE); i++)
            for (uint16_t j = screen_state.y; j < min(GFX_data.y_resolution, screen_state.y + MOUSE_YSIZE); j++) 
                GFX_pdraw_pixel(i, j, screen_state.buffer[(i - screen_state.x) * MOUSE_XSIZE + (j - screen_state.y)]);
                
    screen_state.x = mouse_state.x;
    screen_state.y = mouse_state.y;
    
    for (uint16_t i = screen_state.x; i < min(GFX_data.x_resolution, screen_state.x + MOUSE_XSIZE); i++)
        for (uint16_t j = screen_state.y; j < min(GFX_data.y_resolution, screen_state.y + MOUSE_YSIZE); j++) {
            screen_state.buffer[(i - screen_state.x) * MOUSE_XSIZE + (j - screen_state.y)] = GFX_get_pixel(i, j);

            int32_t color = __cursor_bitmap__[(i - screen_state.x) * MOUSE_XSIZE + (j - screen_state.y)];
            GFX_pdraw_pixel(i, j, color);
        }
}

void i386_mouse_handler(struct Registers* regs) {
    uint8_t status = i386_inb(MOUSE_STATUS);
    while (status & MOUSE_BBIT) {
        if (status & MOUSE_F_BIT) {
            uint8_t state = __mouse_read();
            int8_t x_rel  = __mouse_read();
            int8_t y_rel  = __mouse_read();  

            if (LEFT_BUTTON(state)) mouse_state.leftButton = 1;
            else mouse_state.leftButton = 0;

            if (RIGHT_BUTTON(state)) mouse_state.rightButton = 1;
            else mouse_state.rightButton = 0;

            if (MIDDLE_BUTTON(state)) mouse_state.middleButton = 1;
            else mouse_state.middleButton = 0;

            mouse_state.x += x_rel;
            mouse_state.y -= y_rel;

            if(mouse_state.x < 0) mouse_state.x = 0;
            if(mouse_state.y < 0) mouse_state.y = 0;
            if(mouse_state.x >= GFX_data.x_resolution) mouse_state.x = GFX_data.x_resolution;
            if(mouse_state.y >= GFX_data.y_resolution) mouse_state.y = GFX_data.y_resolution;
        }

        status = i386_inb(MOUSE_STATUS);
    }

    if (_is_mouse_showing) __place_cursor();
}

int i386_init_mouse(int show_mouse) {
    screen_state.x = -1;
    screen_state.y = -1;

    i386_disableInterrupts();

	uint8_t status;
	__mouse_wait(1);
	i386_outb(MOUSE_STATUS, 0xA8);

	__mouse_wait(1);
	i386_outb(MOUSE_STATUS, 0x20);

	__mouse_wait(0);
	status = i386_inb(MOUSE_PORT) | 2;

	__mouse_wait(1);
	i386_outb(MOUSE_STATUS, MOUSE_PORT);

	__mouse_wait(1);
	i386_outb(MOUSE_PORT, status);

	__mouse_write(0xF6);
	__mouse_read();

	__mouse_write(0xF4);
	__mouse_read();

	i386_irq_registerHandler(MOUSE_IRQ, i386_mouse_handler);

    i386_enableInterrupts();
    _is_mouse_showing = show_mouse;
    return 1;
}

//=========================
// Function that detects mouse on device
// 0 - no mouse
// 1 - mouse
int i386_detect_ps2_mouse() {
    if (__mouse_read() != 0xFA) return 0;
    return 1;
}

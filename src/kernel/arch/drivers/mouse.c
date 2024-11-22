#include "../../include/mouse.h" 


mouse_state_t mouse_state;
screen_state_t screen_state;
int32_t __cursor_bitmap__[] = {
    WHITE, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT,
    WHITE, WHITE, TRANSPARENT, TRANSPARENT, TRANSPARENT,
    WHITE, WHITE, WHITE, TRANSPARENT, TRANSPARENT,
    WHITE, WHITE, WHITE, WHITE, TRANSPARENT,
    TRANSPARENT, WHITE, WHITE, TRANSPARENT, TRANSPARENT,
};

int show_mouse = 0;


void i386_mouse_wait(uint8_t a_type) {
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

void i386_mouse_write(uint8_t write) {
	i386_mouse_wait(1);
	i386_outb(MOUSE_STATUS, MOUSE_WRITE);
	i386_mouse_wait(1);
	i386_outb(MOUSE_PORT, write);
}

uint8_t i386_mouse_read() {
	i386_mouse_wait(0);
	return i386_inb(MOUSE_PORT);
}

void i386_mouse_handler(struct Registers* regs) {
    uint8_t status = i386_inb(MOUSE_STATUS);
    while (status & MOUSE_BBIT) {
        if (status & MOUSE_F_BIT) {
            uint8_t state = i386_mouse_read();
            int8_t x_rel  = i386_mouse_read();
            int8_t y_rel  = i386_mouse_read();  

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

    if (show_mouse == 1) place();
}

void place() {
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

int i386_init_mouse() {
    screen_state.x = -1;
    screen_state.y = -1;

    i386_disableInterrupts();

	uint8_t status;
	i386_mouse_wait(1);
	i386_outb(MOUSE_STATUS, 0xA8);

	i386_mouse_wait(1);
	i386_outb(MOUSE_STATUS, 0x20);

	i386_mouse_wait(0);
	status = i386_inb(MOUSE_PORT) | 2;

	i386_mouse_wait(1);
	i386_outb(MOUSE_STATUS, MOUSE_PORT);

	i386_mouse_wait(1);
	i386_outb(MOUSE_PORT, status);

	i386_mouse_write(0xF6);
	i386_mouse_read();

	i386_mouse_write(0xF4);
	i386_mouse_read();

	i386_irq_registerHandler(MOUSE_IRQ, i386_mouse_handler);

    i386_enableInterrupts();

    return 1;
}

//=========================
// Function that detects mouse on device
// 0 - no mouse
// 1 - mouse
int i386_detect_ps2_mouse() {
    unsigned char tmp = i386_mouse_read();
    if(tmp != 0xFA) return 0;
    else return 1;
}

//=========================
// Function that returns mouse state
mouse_state_t i386_mouse_state() {
    return mouse_state;
}
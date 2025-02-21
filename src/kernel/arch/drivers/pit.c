#include "../../include/pit.h"


void i386_pit_init() {
    // Set PIT to generate interrupts at a specific frequency
    uint16_t divisor = 1193180 / TIMER_FREQUENCY_HZ;

    // Set command byte: 0x36 -> channel 0, lobyte/hibyte, rate generator
    i386_outb(PIT_COMMAND_PORT, 0x36);

    // Set low byte and high byte of the divisor
    i386_outb(PIT_DATA_PORT0, (uint8_t)(divisor & 0xFF));
    i386_outb(PIT_DATA_PORT0, (uint8_t)((divisor >> 8) & 0xFF));
}
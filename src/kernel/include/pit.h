#ifndef PIT_H_
#define PIT_H_

#include "x86.h"


// Programmable Interval Timer (PIT) registers
#define PIT_DATA_PORT0 0x40
#define PIT_COMMAND_PORT 0x43

// Frequency to generate timer interrupts (adjust as needed)
#define TIMER_FREQUENCY_HZ 100


void i386_pit_init();

#endif
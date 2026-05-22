#ifndef TIME_H_
#define TIME_H_

#include <stdint.h>

void sleep_s(const uint16_t seconds);
void sleep_ms(const uint32_t milliseconds);
int get_tick();

#endif
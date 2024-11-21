#ifndef DATETIME_H_
#define DATETIME_H_

#include "x86.h"

// Change this each year as needed.
// This is only used if the century register doesn't exist.
#define CURRENT_YEAR    2024
#define CURRENT_CENTURY 21
#define TICK_DELAY      1000000
#define MAX_TICK        0xFFFFFFFC


typedef struct datetime {
    uint8_t datetime_second;
    uint8_t datetime_minute;
    uint8_t datetime_hour;
    uint8_t datetime_day;
    uint8_t datetime_month;
    uint16_t datetime_year;
} datetime_t;

extern datetime_t DTM_datetime;


void _datetime_read_rtc();
void _tick();
int DTM_get_ticks();

#endif
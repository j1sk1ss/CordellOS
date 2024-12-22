#ifndef DATETIME_H_
#define DATETIME_H_

#include "x86.h"

// Change this each year as needed.
// This is only used if the century register doesn't exist.
#define CURRENT_YEAR    2024
#define CURRENT_CENTURY 21
#define TICK_DELAY      1000000
#define MAX_TICK        0xFFFFFFFF


typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} datetime_t;

enum {
    cmos_address = 0x70,
    cmos_data    = 0x71
};


extern datetime_t DTM_datetime;


void _datetime_read_rtc();
void _tick();
int DTM_get_ticks();

#endif
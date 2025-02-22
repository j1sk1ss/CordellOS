#include "../../include/datetime.h"


datetime_t DTM_datetime = {
    .day = 0,
    .hour = 0,
    .minute = 0,
    .month = 0,
    .second = 0,
    .year = CURRENT_YEAR
};


int _get_update_in_progress_flag() {
    i386_outb(cmos_address, 0x0A);
    return (i386_inb(cmos_data) & 0x80);
}
 
uint8_t _get_RTC_register(int reg) {
      i386_outb(cmos_address, reg);
      return i386_inb(cmos_data);
}
 
void _datetime_read_rtc() {
    unsigned char century = 0;
    unsigned char last_second;
    unsigned char last_minute;
    unsigned char last_hour;
    unsigned char last_day;
    unsigned char last_month;
    unsigned char last_year;
    unsigned char last_century;
    unsigned char registerB;

    while (_get_update_in_progress_flag());
    DTM_datetime.second = _get_RTC_register(0x00);
    DTM_datetime.minute = _get_RTC_register(0x02);
    DTM_datetime.hour   = _get_RTC_register(0x04);
    DTM_datetime.day    = _get_RTC_register(0x07);
    DTM_datetime.month  = _get_RTC_register(0x08);
    DTM_datetime.year   = _get_RTC_register(0x09);

    do {
        last_second  = DTM_datetime.second;
        last_minute  = DTM_datetime.minute;
        last_hour    = DTM_datetime.hour;
        last_day     = DTM_datetime.day;
        last_month   = DTM_datetime.month;
        last_year    = DTM_datetime.year;
        last_century = century;

        while (_get_update_in_progress_flag());
        DTM_datetime.second = _get_RTC_register(0x00);
        DTM_datetime.minute = _get_RTC_register(0x02);
        DTM_datetime.hour   = _get_RTC_register(0x04);
        DTM_datetime.day    = _get_RTC_register(0x07);
        DTM_datetime.month  = _get_RTC_register(0x08);
        DTM_datetime.year   = _get_RTC_register(0x09);
    } while (
        (last_second != DTM_datetime.second) || 
        (last_minute != DTM_datetime.minute) || 
        (last_hour != DTM_datetime.hour) ||
        (last_day != DTM_datetime.day) || 
        (last_month != DTM_datetime.month) || 
        (last_year != DTM_datetime.year) ||
        (last_century != century)
    );

    registerB = _get_RTC_register(0x0B);

    if (!(registerB & 0x04)) {
        DTM_datetime.second = (DTM_datetime.second & 0x0F) + ((DTM_datetime.second / 16) * 10);
        DTM_datetime.minute = (DTM_datetime.minute & 0x0F) + ((DTM_datetime.minute / 16) * 10);
        DTM_datetime.hour   = ((DTM_datetime.hour & 0x0F) + (((DTM_datetime.hour & 0x70) / 16) * 10)) | (DTM_datetime.hour & 0x80);
        DTM_datetime.day    = (DTM_datetime.day & 0x0F) + ((DTM_datetime.day / 16) * 10);
        DTM_datetime.month  = (DTM_datetime.month & 0x0F) + ((DTM_datetime.month / 16) * 10);
        DTM_datetime.year   = (DTM_datetime.year & 0x0F) + ((DTM_datetime.year / 16) * 10);
    }

    if (!(registerB & 0x02) && (DTM_datetime.hour & 0x80)) 
        DTM_datetime.hour = ((DTM_datetime.hour & 0x7F) + 12) % 24;

    DTM_datetime.year += (CURRENT_YEAR / 100) * 100;
    if (DTM_datetime.year < CURRENT_YEAR) DTM_datetime.year += 100;
}

static int ticks = 0;
void _tick() {
    int temp_ticks = 0;
    while (1) {
        if (++temp_ticks > TICK_DELAY) {
            temp_ticks = 0;
            ticks++;
        }

        if (ticks > MAX_TICK) ticks = 0;
    }
}

int DTM_get_ticks() {
    return ticks;
}
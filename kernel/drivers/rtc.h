#ifndef RTC_H
#define RTC_H
#include <stdint.h>

// Date and time from the CMOS real-time clock (battery-backed chip on the
// motherboard). It holds whatever the firmware/OS set: on a PC that dual-boots
// Windows that's usually LOCAL time, not UTC.
struct rtc_time {
    uint8_t  second, minute, hour;   // 24-hour clock
    uint8_t  day, month;
    uint16_t year;
};

void rtc_read(struct rtc_time* t);

#endif
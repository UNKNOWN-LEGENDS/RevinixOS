#ifndef _TIME_H
#define _TIME_H

typedef long time_t;           // seconds since 1970-01-01 00:00:00

struct tm {
    int tm_sec;                // 0-59
    int tm_min;                // 0-59
    int tm_hour;               // 0-23
    int tm_mday;               // day of the month, 1-31
    int tm_mon;                // month, 0-11 (0 = January)
    int tm_year;               // years since 1900
    int tm_wday;               // day of the week, 0-6 (0 = Sunday)
    int tm_yday;               // day of the year, 0-365
    int tm_isdst;              // always 0: no time zones yet
};

// Seconds since 1970 from the hardware clock. Also stored in *t if t != NULL.
// The clock holds whatever the BIOS/other OS set - often local time, not UTC.
time_t time(time_t* t);

// Split a time_t into date and time fields. Returns a pointer to one static
// struct that the next call overwrites (as in every C library).
struct tm* gmtime(const time_t* t);

#endif
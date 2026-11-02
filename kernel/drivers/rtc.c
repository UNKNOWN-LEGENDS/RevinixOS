#include "rtc.h"
#include "../arch/x86_64/io.h"

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

static uint8_t cmos_read(uint8_t reg) {
    outb(CMOS_ADDR, 0x80 | reg);     // bit 7 set = keep NMIs disabled while we read
    return inb(CMOS_DATA);
}

// Status register A bit 7: the chip is updating its registers right now.
static int update_in_progress(void) {
    return cmos_read(0x0A) & 0x80;
}

static void read_raw(struct rtc_time* t) {
    // An update takes under 2 ms; bounded so a broken chip can't hang us.
    for (int i = 0; i < 100000 && update_in_progress(); i++) { }
    t->second = cmos_read(0x00);
    t->minute = cmos_read(0x02);
    t->hour   = cmos_read(0x04);
    t->day    = cmos_read(0x07);
    t->month  = cmos_read(0x08);
    t->year   = cmos_read(0x09);
}

static uint8_t bcd_to_bin(uint8_t v) { return (uint8_t)((v & 0x0F) + (v >> 4) * 10); }

void rtc_read(struct rtc_time* t) {
    // Read until two reads in a row agree, so we never mix the seconds from
    // before a tick with the minutes from after it.
    struct rtc_time a, b;
    read_raw(&b);
    do {
        a = b;
        read_raw(&b);
    } while (a.second != b.second || a.minute != b.minute || a.hour != b.hour ||
             a.day != b.day || a.month != b.month || a.year != b.year);

    uint8_t regB = cmos_read(0x0B);
    uint8_t pm   = b.hour & 0x80;            // 12-hour mode keeps PM in bit 7
    b.hour &= 0x7F;

    if (!(regB & 0x04)) {                     // bit 2 clear = values are BCD
        b.second = bcd_to_bin(b.second);
        b.minute = bcd_to_bin(b.minute);
        b.hour   = bcd_to_bin(b.hour);
        b.day    = bcd_to_bin(b.day);
        b.month  = bcd_to_bin(b.month);
        b.year   = bcd_to_bin((uint8_t)b.year);
    }
    if (!(regB & 0x02) && pm)                 // bit 1 clear = 12-hour mode
        b.hour = (uint8_t)((b.hour % 12) + 12);
    else if (!(regB & 0x02) && b.hour == 12)
        b.hour = 0;                           // 12 AM = 00

    b.year += 2000;                           // the chip only stores 2 digits
    *t = b;
}
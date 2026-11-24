#include <time.h>

// gmtime: the inverse of the kernel's days_from_civil (Howard Hinnant's
// civil_from_days). Works in "March years" so the leap day comes last.
struct tm* gmtime(const time_t* tp) {
    static struct tm tm;
    long t    = *tp;
    long days = t / 86400;
    long secs = t % 86400;
    if (secs < 0) { secs += 86400; days--; }          // times before 1970

    tm.tm_hour = (int)(secs / 3600);
    tm.tm_min  = (int)(secs / 60 % 60);
    tm.tm_sec  = (int)(secs % 60);
    tm.tm_wday = (int)((days % 7 + 11) % 7);          // 1970-01-01 was a Thursday (4)

    long z   = days + 719468;                         // days since 0000-03-01
    long era = (z >= 0 ? z : z - 146096) / 146097;    // 400-year era
    long doe = z - era * 146097;                      // day of era
    long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);   // day of the March year
    long mp  = (5 * doy + 2) / 153;                   // month, 0 = March
    long y   = yoe + era * 400;
    int  m   = (int)(mp < 10 ? mp + 3 : mp - 9);      // 1-12
    if (m <= 2) y++;

    tm.tm_mday  = (int)(doy - (153 * mp + 2) / 5 + 1);
    tm.tm_mon   = m - 1;
    tm.tm_year  = (int)(y - 1900);
    int leap    = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    tm.tm_yday  = (int)(m <= 2 ? doy - 306 : doy + 59 + leap);   // Jan 1 = 0
    tm.tm_isdst = 0;
    return &tm;
}
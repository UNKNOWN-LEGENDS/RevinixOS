// timetest.c - getpid, sleeping, the uptime clock and the wall clock.
// Exit code = number of failed checks.
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

static int failures = 0;

static void check(int ok, const char* what) {
    printf("[timetest %d] %-34s %s\n", getpid(), what, ok ? "ok" : "FAILED");
    if (!ok) failures++;
}

static const char* const wday_name[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

int main(void) {
    int pid = getpid();
    check(pid > 0, "getpid() gives a task id");
    check(getpid() == pid, "getpid() is stable");

    // 1. the uptime clock moves forward
    long t0 = uptime_ms();
    check(t0 > 0, "uptime_ms() is running");

    // 2. msleep(500) sleeps at least 500 ms, and not much more
    t0 = uptime_ms();
    msleep(500);
    long slept = uptime_ms() - t0;
    printf("[timetest %d] msleep(500) took %ld ms\n", pid, slept);
    check(slept >= 500 && slept <= 600, "msleep(500) takes 500-600 ms");

    // 3. msleep(0) only gives up the time slice
    t0 = uptime_ms();
    msleep(0);
    check(uptime_ms() - t0 < 50, "msleep(0) returns at once");

    // 4. a sleep longer than a day is refused
    check(msleep(25u * 60 * 60 * 1000) == -1 && errno == EINVAL, "msleep(25 hours) refused (EINVAL)");

    // 5. the wall clock: a sane date, and it agrees with sleep(2)
    time_t stored = 0;
    time_t now = time(&stored);
    check(now == stored, "time(&t) also stores the time");
    check(now > 1704067200L, "time() is after 2024-01-01");

    struct tm* tm = gmtime(&now);
    printf("[timetest %d] RTC says %s %04d-%02d-%02d %02d:%02d:%02d (day %d of the year)\n",
           pid, wday_name[tm->tm_wday], tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
           tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_yday + 1);

    t0 = uptime_ms();
    sleep(2);
    long ms = uptime_ms() - t0;
    long secs = time(0) - now;
    printf("[timetest %d] sleep(2) took %ld ms, the RTC moved %ld s\n", pid, ms, secs);
    check(ms >= 2000 && ms <= 2100, "sleep(2) takes 2000-2100 ms");
    check(secs >= 2 && secs <= 3, "the RTC agrees (2-3 s later)");

    printf("[timetest %d] %d check(s) failed\n", pid, failures);
    return failures;
}
#ifndef _UNISTD_H
#define _UNISTD_H
#include <stddef.h>
#include <stdint.h>

typedef long ssize_t;
typedef int pid_t;

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

ssize_t read(int fd, void* buf, size_t count);
ssize_t write(int fd, const void* buf, size_t count);
int     close(int fd);

// The heap: brk() sets its end, sbrk() moves it by `increment` bytes and
// returns the OLD end (so sbrk(0) tells you where it is). Usually you want
// malloc() instead.
int     brk(void* addr);
void*   sbrk(intptr_t increment);
pid_t    getpid(void);
unsigned sleep(unsigned seconds);     // always returns 0 (nothing can wake us early yet)

// Not POSIX, RevinixOS only:
int      msleep(unsigned ms);         // sleep at least `ms` milliseconds
long     uptime_ms(void);             // milliseconds since boot (10 ms steps)

#endif
// Thin C wrappers around the RevinixOS syscalls, POSIX style: on failure
// they set errno and return -1.
#include <sys/syscall.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

int errno = 0;

static long check(long r) {
    if (r < 0) { errno = (int)-r; return -1; }
    return r;
}

ssize_t read(int fd, void* buf, size_t count) {
    return check(__syscall3(SYS_READ, fd, (long)buf, (long)count));
}

ssize_t write(int fd, const void* buf, size_t count) {
    return check(__syscall3(SYS_WRITE, fd, (long)buf, (long)count));
}

int open(const char* path, int flags) {
    return (int)check(__syscall3(SYS_OPEN, (long)path, flags, 0));
}

int close(int fd) {
    return (int)check(__syscall3(SYS_CLOSE, fd, 0, 0));
}

void exit(int code) {
    __syscall3(SYS_EXIT, code, 0, 0);
    for (;;) { }                    // the kernel never returns here
}
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

// The current program break, cached so sbrk() doesn't need to ask each time.
static char* cur_brk = 0;

int brk(void* addr) {
    long r = __syscall3(SYS_BRK, (long)addr, 0, 0);    // returns the break it ended up at
    cur_brk = (char*)r;
    if (r != (long)addr) { errno = ENOMEM; return -1; }
    return 0;
}

void* sbrk(intptr_t increment) {
    if (!cur_brk) cur_brk = (char*)__syscall3(SYS_BRK, 0, 0, 0);
    char* old = cur_brk;
    if (increment == 0) return old;
    if (brk(old + increment) != 0) return (void*)-1;
    return old;
}

void exit(int code) {
    __syscall3(SYS_EXIT, code, 0, 0);
    for (;;) { }                    // the kernel never returns here
}
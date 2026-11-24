// chello.c - the first RevinixOS program written in C.
// Exercises: printf, globals in .data and .bss, recursion on the stack,
// SSE floating-point math, reading the keyboard, open/read/close, errno,
// and main's return value becoming the exit code.
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int counter = 41;                   // initialised global -> .data (rw-)
static char name[64];               // zeroed global      -> .bss  (rw-)
static const char* greeting = "Hello from C";

static int fib(int n) {             // recursion: real stack frames
    return n < 2 ? n : fib(n - 1) + fib(n - 2);
}

int main(void) {
    printf("%s! main() is at %p, running in Ring 3.\n", greeting, (void*)main);

    counter++;
    printf("counter = %d (a global variable in .data)\n", counter);
    printf("fib(20) = %d | hex %x | negative %d | width [%5d] [%-5d] [%05d]\n",
           fib(20), 255, -42, 7, 7, 7);

    volatile double a = 3.5, b = 2.0;   // compiled to SSE instructions
    printf("SSE math: 3.5 * 2.0 = %d\n", (int)(a * b));

    printf("What's your name? ");
    ssize_t n = read(STDIN_FILENO, name, sizeof(name) - 1);
    if (n > 0) {
        name[n] = '\0';
        if (name[n - 1] == '\n') name[n - 1] = '\0';
    }
    printf("Nice to meet you, %s. That's %d letters.\n", name, (int)strlen(name));

    int fd = open("README.TXT", O_RDONLY);
    if (fd >= 0) {
        char line[48];
        ssize_t got = read(fd, line, sizeof(line) - 1);
        line[got > 0 ? got : 0] = '\0';
        char* nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        printf("First line of README.TXT (fd %d): \"%s\"\n", fd, line);
        close(fd);
    }

    if (open("NOPE.TXT", O_RDONLY) < 0)
        printf("open(\"NOPE.TXT\") failed as expected: %s (errno %d)\n", strerror(errno), errno);

    return 7;                       // becomes the exit code
}
// memtest.c - exercises the heap: brk/sbrk in the kernel, malloc/free/
// calloc/realloc in the libc. Exit code = number of failed checks.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

static int failures = 0;

static void check(int ok, const char* what) {
    printf("[memtest] %-34s %s\n", what, ok ? "ok" : "FAILED");
    if (!ok) failures++;
}

#define SMALL 1000
static unsigned char* small[SMALL];

int main(void) {
    char* start = sbrk(0);
    printf("[memtest] heap starts at %p\n", start);

    // 1. many small blocks: aligned, writable, nothing overlaps
    int aligned = 1, intact = 1;
    for (int i = 0; i < SMALL; i++) {
        small[i] = malloc(100);
        if (!small[i] || ((uintptr_t)small[i] & 15)) aligned = 0;
        if (small[i]) memset(small[i], i & 0xFF, 100);
    }
    for (int i = 0; i < SMALL; i++)
        for (int j = 0; j < 100; j++)
            if (small[i][j] != (i & 0xFF)) intact = 0;
    check(aligned, "1000 x malloc(100), 16-aligned");
    check(intact,  "no block overwrote another");
    char* after_small = sbrk(0);
    for (int i = 0; i < SMALL; i += 2) free(small[i]);   // every other one ...
    for (int i = 1; i < SMALL; i += 2) free(small[i]);   // ... then the rest
    check(sbrk(0) == start, "freeing them all gives memory back");

    // 2. one big block grows the heap, freeing it shrinks it again
    char* big = malloc(300 * 1024);
    char* grown = sbrk(0);
    int big_ok = big != 0;
    for (int i = 0; big && i < 300 * 1024; i++) big[i] = (char)i;
    for (int i = 0; big && i < 300 * 1024; i++) if (big[i] != (char)i) big_ok = 0;
    check(big_ok && grown - start >= 300 * 1024, "malloc(300 KB) grows the heap");
    free(big);
    check(sbrk(0) == start, "free() shrinks it back");
    printf("[memtest] break: start %p, after small %p, after big %p, now %p\n",
           start, after_small, grown, sbrk(0));

    // 3. calloc must zero even memory that was used before
    char* dirty = malloc(16 * 1024);
    if (dirty) memset(dirty, 0xAA, 16 * 1024);
    free(dirty);
    int* zeros = calloc(4096, sizeof(int));
    int all_zero = zeros != 0;
    for (int i = 0; zeros && i < 4096; i++) if (zeros[i]) all_zero = 0;
    check(all_zero, "calloc() returns zeroed memory");
    free(zeros);

    // 4. realloc keeps the contents while moving to a bigger block
    char* s = malloc(16);
    strcpy(s, "hello");
    s = realloc(s, 100 * 1024);
    check(s && strcmp(s, "hello") == 0, "realloc() keeps the data");
    free(s);

    // 5. limits: the kernel must refuse these
    errno = 0;
    check(sbrk(1L << 30) == (void*)-1 && errno == ENOMEM, "sbrk(1 GB) refused (ENOMEM)");
    check(brk((void*)0x1000) == -1, "brk() below the heap refused");
    check(malloc((size_t)1 << 30) == 0, "malloc(1 GB) returns NULL");
    check(sbrk(0) == start, "heap is empty again at the end");

    printf("[memtest] %d check(s) failed\n", failures);
    return failures;
}
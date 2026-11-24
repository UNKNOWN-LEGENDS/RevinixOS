#include <string.h>
#include <errno.h>

size_t strlen(const char* s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char* a, const char* b, size_t n) {
    for (; n > 0; n--, a++, b++) {
        if (*a != *b) return (unsigned char)*a - (unsigned char)*b;
        if (*a == '\0') return 0;
    }
    return 0;
}

char* strcpy(char* dst, const char* src) {
    char* d = dst;
    while ((*d++ = *src++)) { }
    return dst;
}

char* strchr(const char* s, int c) {
    for (; *s; s++) if (*s == (char)c) return (char*)s;
    return c == 0 ? (char*)s : 0;
}

// The compiler may call memcpy / memset / memmove / memcmp itself (for struct
// copies and big zero-fills), so these four must exist even if you never
// call them by name.
void* memcpy(void* dst, const void* src, size_t n) {
    unsigned char* d = dst;
    const unsigned char* s = src;
    while (n--) *d++ = *s++;
    return dst;
}

void* memmove(void* dst, const void* src, size_t n) {
    unsigned char* d = dst;
    const unsigned char* s = src;
    if (d < s) { while (n--) *d++ = *s++; }
    else       { d += n; s += n; while (n--) *--d = *--s; }
    return dst;
}

void* memset(void* dst, int c, size_t n) {
    unsigned char* d = dst;
    while (n--) *d++ = (unsigned char)c;
    return dst;
}

int memcmp(const void* a, const void* b, size_t n) {
    const unsigned char* x = a;
    const unsigned char* y = b;
    for (; n > 0; n--, x++, y++)
        if (*x != *y) return *x - *y;
    return 0;
}

const char* strerror(int err) {
    switch (err) {
        case ENOENT:       return "no such file";
        case EBADF:        return "bad file descriptor";
        case EFAULT:       return "bad address";
        case EINVAL:       return "invalid argument";
        case EMFILE:       return "too many open files";
        case ENAMETOOLONG: return "name too long";
        case ENOSYS:       return "not implemented";
        default:           return "unknown error";
    }
}
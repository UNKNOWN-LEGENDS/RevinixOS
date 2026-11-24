#include <stdio.h>
#include <string.h>
#include <unistd.h>

// ---- the formatter: everything else is built on vsnprintf ----

struct out {
    char*  buf;
    size_t size;        // capacity including the final NUL
    size_t len;         // characters produced so far (may exceed size)
};

static void emit(struct out* o, char c) {
    if (o->len + 1 < o->size) o->buf[o->len] = c;
    o->len++;
}

// Write the digits of v in `base`, padded to `width`.
static void emit_number(struct out* o, unsigned long long v, int base, int upper,
                        int negative, int width, int zero_pad, int left) {
    const char* digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char tmp[24];
    int n = 0;
    do { tmp[n++] = digits[v % base]; v /= base; } while (v);

    int len = n + (negative ? 1 : 0);
    if (!left && !zero_pad) for (; len < width; width--) emit(o, ' ');
    if (negative) emit(o, '-');
    if (!left && zero_pad)  for (; len < width; width--) emit(o, '0');
    while (n) emit(o, tmp[--n]);
    if (left) for (; len < width; width--) emit(o, ' ');
}

int vsnprintf(char* buf, size_t size, const char* fmt, va_list ap) {
    struct out o = { buf, size, 0 };

    for (; *fmt; fmt++) {
        if (*fmt != '%') { emit(&o, *fmt); continue; }
        fmt++;

        int left = 0, zero_pad = 0, width = 0, longs = 0;
        for (;; fmt++) {                                   // flags
            if (*fmt == '-') left = 1;
            else if (*fmt == '0') zero_pad = 1;
            else break;
        }
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        while (*fmt == 'l') { longs++; fmt++; }            // l, ll

        switch (*fmt) {
        case 'd': case 'i': {
            long long v = longs ? va_arg(ap, long long) : va_arg(ap, int);
            unsigned long long mag = v < 0 ? -(unsigned long long)v : (unsigned long long)v;
            emit_number(&o, mag, 10, 0, v < 0, width, zero_pad, left);
            break;
        }
        case 'u': case 'x': case 'X': {
            unsigned long long v = longs ? va_arg(ap, unsigned long long) : va_arg(ap, unsigned int);
            emit_number(&o, v, *fmt == 'u' ? 10 : 16, *fmt == 'X', 0, width, zero_pad, left);
            break;
        }
        case 'p':
            emit(&o, '0'); emit(&o, 'x');
            emit_number(&o, (unsigned long long)va_arg(ap, void*), 16, 0, 0, 0, 0, 0);
            break;
        case 'c': {
            char c = (char)va_arg(ap, int);
            if (!left) for (int w = 1; w < width; w++) emit(&o, ' ');
            emit(&o, c);
            if (left)  for (int w = 1; w < width; w++) emit(&o, ' ');
            break;
        }
        case 's': {
            const char* s = va_arg(ap, const char*);
            if (!s) s = "(null)";
            int len = (int)strlen(s);
            if (!left) for (int w = len; w < width; w++) emit(&o, ' ');
            while (*s) emit(&o, *s++);
            if (left)  for (int w = len; w < width; w++) emit(&o, ' ');
            break;
        }
        case '%':
            emit(&o, '%');
            break;
        case '\0':                                          // stray % at the end
            fmt--;
            break;
        default:                                            // unknown: print it as-is
            emit(&o, '%');
            emit(&o, *fmt);
            break;
        }
    }

    if (size) buf[o.len < size ? o.len : size - 1] = '\0';
    return (int)o.len;
}

int snprintf(char* buf, size_t size, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}

// printf formats into a stack buffer and hands it to write() in one call,
// so a line never gets split up by other programs' output.
int printf(const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    int len = n < (int)sizeof(buf) ? n : (int)sizeof(buf) - 1;
    write(STDOUT_FILENO, buf, (size_t)len);
    return n;
}

int puts(const char* s) {
    write(STDOUT_FILENO, s, strlen(s));
    write(STDOUT_FILENO, "\n", 1);
    return 0;
}

int putchar(int c) {
    char ch = (char)c;
    write(STDOUT_FILENO, &ch, 1);
    return c;
}
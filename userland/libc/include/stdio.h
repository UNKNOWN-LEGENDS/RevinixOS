#ifndef _STDIO_H
#define _STDIO_H
#include <stddef.h>
#include <stdarg.h>

// printf supports: %d %i %u %x %X %p %s %c %%, the 'l' / 'll' size
// prefixes (%ld, %lu, %lx), a width (%5d), '0' padding (%05d) and '-' to
// left-align (%-8s). No floating point.
int printf(const char* fmt, ...)                        __attribute__((format(printf, 1, 2)));
int snprintf(char* buf, size_t size, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
int vsnprintf(char* buf, size_t size, const char* fmt, va_list ap);
int puts(const char* s);           // writes s and a newline
int putchar(int c);

#endif
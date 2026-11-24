#ifndef _STDLIB_H
#define _STDLIB_H
#include <stddef.h>

void exit(int code) __attribute__((noreturn));
int  atoi(const char* s);
int  abs(int x);

void* malloc(size_t size);
void  free(void* ptr);
void* calloc(size_t count, size_t size);
void* realloc(void* ptr, size_t size);

#endif
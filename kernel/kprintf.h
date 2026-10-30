#ifndef KPRINTF_H
#define KPRINTF_H
#include <stdint.h>

void kprintf(const char* fmt, ...);

#define KLOG_SIZE 16384

uint32_t klog_snapshot(char* dst, uint32_t max);

int klog_wrapped(void);

void console_write(const char* s, uint32_t len);

#endif

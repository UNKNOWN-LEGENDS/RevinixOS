#ifndef IO_H
#define IO_H
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t r;
    __asm__ volatile ("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint16_t inw(uint16_t port) {
    uint16_t r;
    __asm__ volatile ("inw %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}
// read `count` 16-bit words from `port` into `buf`
static inline void insw(uint16_t port, void* buf, uint32_t count) {
    __asm__ volatile ("rep insw" : "+D"(buf), "+c"(count) : "d"(port) : "memory");
}
// write `count` 16-bit words from `buf` to `port`
static inline void outsw(uint16_t port, const void* buf, uint32_t count) {
    __asm__ volatile ("rep outsw" : "+S"(buf), "+c"(count) : "d"(port) : "memory");
}
#endif
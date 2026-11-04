#ifndef IRQFLAGS_H
#define IRQFLAGS_H
#include <stdint.h>

// Disable interrupts and return the previous RFLAGS, so irq_restore() puts
// the interrupt flag back exactly as it was (never turns IRQs on by accident).
static inline uint64_t irq_save(void) {
    uint64_t f;
    __asm__ volatile ("pushfq; pop %0; cli" : "=r"(f) : : "memory");
    return f;
}

static inline void irq_restore(uint64_t f) {
    __asm__ volatile ("push %0; popfq" : : "r"(f) : "memory", "cc");
}

#endif
#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>

// The PIT fires IRQ0 this many times per second (see irq.c).
#define TIMER_HZ 100
#define MS_PER_TICK (1000 / TIMER_HZ)      // 10 ms

// Timer interrupts since boot.
uint64_t get_ticks(void);

#endif
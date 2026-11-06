#ifndef FPU_H
#define FPU_H
#include <stdint.h>

// Size of the FXSAVE area: x87 + MMX + all 16 XMM registers + MXCSR.
#define FPU_STATE_SIZE 512

// Turn on x87 + SSE for user programs and record a clean starting state.
void  fpu_init(void);

// A 16-byte aligned 512-byte save area pre-filled with the clean state
// (every new task starts with sane FPU/SSE settings). Free with fpu_free_state.
void* fpu_alloc_state(void);
void  fpu_free_state(void* state);

// Save / load ALL FPU+SSE registers to / from a save area (must be 16-aligned).
static inline void fpu_save(void* state) {
    __asm__ volatile ("fxsave64 (%0)" : : "r"(state) : "memory");
}
static inline void fpu_restore(void* state) {
    __asm__ volatile ("fxrstor64 (%0)" : : "r"(state) : "memory");
}

#endif
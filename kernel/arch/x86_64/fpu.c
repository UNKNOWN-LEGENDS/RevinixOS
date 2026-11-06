#include "fpu.h"
#include "../../mm/heap.h"
#include "../../kprintf.h"

#define CR0_MP  (1ull << 1)    // monitor coprocessor
#define CR0_EM  (1ull << 2)    // emulate FPU: must be 0, or every FPU/SSE op traps
#define CR0_TS  (1ull << 3)    // task switched: lazy-switch trap, we don't use it
#define CR4_OSFXSR     (1ull << 9)    // OS supports FXSAVE/FXRSTOR -> enables SSE
#define CR4_OSXMMEXCPT (1ull << 10)   // OS handles SSE exceptions (#XM)

// Snapshot of a freshly initialised FPU/SSE unit; copied into every new task.
static uint8_t clean_state[FPU_STATE_SIZE] __attribute__((aligned(16)));

void fpu_init(void) {
    uint64_t cr0, cr4;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(CR0_EM | CR0_TS);
    cr0 |= CR0_MP;
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0));

    __asm__ volatile ("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_OSFXSR | CR4_OSXMMEXCPT;
    __asm__ volatile ("mov %0, %%cr4" : : "r"(cr4));

    __asm__ volatile ("fninit");                     // x87 to its reset state
    uint32_t mxcsr = 0x1F80;                         // SSE: all exceptions masked
    __asm__ volatile ("ldmxcsr %0" : : "m"(mxcsr));

    fpu_save(clean_state);
    kprintf("fpu: x87 + SSE enabled, %d-byte save area per task\n", FPU_STATE_SIZE);
}

// kmalloc only guarantees 8-byte alignment; FXSAVE needs 16. Over-allocate,
// round up, and keep the original pointer just below the aligned area.
void* fpu_alloc_state(void) {
    uint8_t* raw = (uint8_t*)kmalloc(FPU_STATE_SIZE + 16 + sizeof(void*));
    if (!raw) return 0;
    uint64_t aligned = ((uint64_t)raw + sizeof(void*) + 15) & ~15ull;
    ((void**)aligned)[-1] = raw;
    uint8_t* s = (uint8_t*)aligned;
    for (int i = 0; i < FPU_STATE_SIZE; i++) s[i] = clean_state[i];
    return s;
}

void fpu_free_state(void* state) {
    if (state) kfree(((void**)state)[-1]);
}
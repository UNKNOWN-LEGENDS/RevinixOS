#include "uaccess.h"
#include "vmm.h"
#include "../syscall/syscall.h"

// The address space of whoever made the syscall is the one in CR3 right now:
// syscalls run on the calling process's page tables.
static uint64_t current_pml4(void) {
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    return cr3 & 0x000FFFFFFFFFF000ull;
}

int user_range_ok(uint64_t uaddr, uint64_t n, int write) {
    return vmm_user_range_ok(current_pml4(), uaddr, n, write);
}

int copy_from_user(void* dst, uint64_t user_src, uint64_t n) {
    if (!vmm_user_range_ok(current_pml4(), user_src, n, 0)) return -EFAULT;
    const uint8_t* s = (const uint8_t*)user_src;
    uint8_t* d = (uint8_t*)dst;
    for (uint64_t i = 0; i < n; i++) d[i] = s[i];
    return 0;
}

int copy_to_user(uint64_t user_dst, const void* src, uint64_t n) {
    if (!vmm_user_range_ok(current_pml4(), user_dst, n, 1)) return -EFAULT;
    const uint8_t* s = (const uint8_t*)src;
    uint8_t* d = (uint8_t*)user_dst;
    for (uint64_t i = 0; i < n; i++) d[i] = s[i];
    return 0;
}
#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H

// RevinixOS syscall numbers (must match kernel/syscall/syscall.h).
#define SYS_WRITE 1
#define SYS_EXIT  2
#define SYS_OPEN  3
#define SYS_READ  4
#define SYS_CLOSE 5
#define SYS_BRK 6

// Raw syscall: rax = number, rdi/rsi/rdx = arguments, result in rax
// (negative = -errno). The kernel preserves every other register.
static inline long __syscall3(long n, long a, long b, long c) {
    long ret;
    __asm__ volatile ("int $0x80"
                      : "=a"(ret)
                      : "a"(n), "D"(a), "S"(b), "d"(c)
                      : "memory");
    return ret;
}

#endif
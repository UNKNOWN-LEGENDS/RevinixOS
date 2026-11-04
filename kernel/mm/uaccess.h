#ifndef UACCESS_H
#define UACCESS_H
#include <stdint.h>

// The only sanctioned way for the kernel to touch memory a user program
// pointed it at. Both check the WHOLE range against the current address space
// first and copy nothing if any byte is bad.
// Return 0 on success, -EFAULT if the range isn't valid user memory.
int copy_from_user(void* dst, uint64_t user_src, uint64_t n);
// 1 if [uaddr, uaddr+n) is valid user memory (writable too if `write`), else 0.
int user_range_ok(uint64_t uaddr, uint64_t n, int write);
int copy_to_user(uint64_t user_dst, const void* src, uint64_t n);

#endif
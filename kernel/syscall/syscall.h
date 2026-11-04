#ifndef SYSCALL_H
#define SYSCALL_H

#include<stdint.h>

#define SYS_WRITE 1
#define SYS_EXIT 2

// Error codes, returned to user programs as negative numbers (Linux style).
#define EFAULT 14       // bad address
#define EINVAL 22       // invalid argument

void syscall_init(void);

#endif
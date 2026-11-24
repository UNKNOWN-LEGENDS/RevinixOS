#ifndef SYSCALL_H
#define SYSCALL_H

#include<stdint.h>

#define SYS_WRITE 1
#define SYS_EXIT 2
#define SYS_OPEN 3
#define SYS_READ 4
#define SYS_CLOSE 5
#define SYS_BRK 6
#define SYS_GETPID 7
#define SYS_SLEEP 8
#define SYS_UPTIME 9
#define SYS_TIME 10

#define O_RDONLY 0      // the only open() mode for now (FAT32 is read-only)

// Error codes, returned to user programs as negative numbers (Linux style).
#define EBADF 9
#define EFAULT 14       // bad address
#define EINVAL 22       // invalid argument
#define ENOENT 2        // no such file
#define EMFILE 24       // too many open files (fd table full)
#define ENAMETOOLONG 36 // path too long
#define ENOSYS 38       // no such syscall / not implemented

void syscall_init(void);

#endif
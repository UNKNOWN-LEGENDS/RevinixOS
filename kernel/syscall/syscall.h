#ifndef SYSCALL_H
#define SYSCALL_H

#include<stdint.h>

#define SYS_WRITE 1
#define SYS_EXIT 2

void syscall_init(void);

#endif
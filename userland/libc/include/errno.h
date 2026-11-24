#ifndef _ERRNO_H
#define _ERRNO_H

// Set by a failing libc call (which then returns -1), like every C library.
extern int errno;

#define ENOENT        2
#define EBADF         9
#define ENOMEM       12
#define EFAULT       14
#define EINVAL       22
#define EMFILE       24
#define ENAMETOOLONG 36
#define ENOSYS       38

#endif
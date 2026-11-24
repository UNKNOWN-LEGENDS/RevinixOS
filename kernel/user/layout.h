#ifndef USER_LAYOUT_H
#define USER_LAYOUT_H

// Where things live in every user process's address space (the lower half).
//
//   0x0000000000000000 - 0x00000000003FFFFF   never mapped: NULL / small-pointer bugs fault
//   0x0000000000400000 - ...                  program image: code, rodata, data, bss
//                                             (userland/user.ld links programs here)
//   ... up to USER_IMAGE_MAX                  free (the heap will grow here in 2b.6)
//   USER_STACK_GUARD   (one page)             never mapped: a stack overflow faults here
//   USER_STACK_BOTTOM  - USER_STACK_TOP       the stack, 64 KB, grows downward
//   USER_STACK_TOP     - 0x00007FFFFFFFFFFF   never mapped (top page of user space)

#define USER_IMAGE_MIN     0x0000000000400000ull
#define USER_IMAGE_MAX     0x0000400000000000ull     // images must end below 64 TB

#define USER_STACK_TOP     0x00007FFFFFFFF000ull
#define USER_STACK_PAGES   16                        // 64 KB
#define USER_STACK_BOTTOM  (USER_STACK_TOP - USER_STACK_PAGES * 0x1000ull)
#define USER_STACK_GUARD   (USER_STACK_BOTTOM - 0x1000ull)

#endif
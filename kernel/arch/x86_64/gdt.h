#ifndef GDT_H
#define GDT_H

#include <stdint.h>

//segment selectors (offsets into the GDT). these are the values you load
//into segment registers / use in the IDT later.

#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE 0x18
#define GDT_USER_DATA 0x20
#define GDT_TSS 0x28

void gdt_init(void);
void gdt_debug(void);

#endif
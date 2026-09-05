#ifndef VGA_H
#define VGA_H

#include<stdint.h>

void vga_clear(void);
void vga_print(const char* s);
void vga_print_at(const char* s, int row, int col);

#endif
#ifndef FBCON_H
#define FBCON_H
#include <stdint.h>

// A text console drawn into a rectangle of the graphics screen.
// Once active, vga.c forwards all text output here.
void fbcon_init(int x, int y, int w, int h, uint32_t fg, uint32_t bg);
int  fbcon_active(void);
void fbcon_putc(char c);
void fbcon_clear(void);
void fbcon_write_at(const char* s, int row, int col);

#endif
#include "vga.h"
#include "../mm/vmm.h"
#include<stdint.h>

#define VGA_MEM ((volatile uint16_t*)(0xB8000 + HHDM_OFFSET))
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

//attribute byte: 0x0F = white text on black background
#define VGA_COLOR 0x0F

static int cursor_row = 0;
static int cursor_col = 0;

static uint16_t vga_entry(char c) {
    return (uint16_t)c | (VGA_COLOR << 8);
}

void vga_clear(void) {
    for (int i=0; i<VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_MEM[i] = vga_entry(' ');
    }
    cursor_row = 0;
    cursor_col = 0;
}

void vga_print_at(const char* s, int row, int col) {
    int offset = row * VGA_WIDTH + col;
    for (int i=0; s[i]!='\0'; i++) {
        VGA_MEM[offset++] = vga_entry(s[i]);
    }
}

void vga_print(const char* s) {
    for (int i=0; s[i]!='\0'; i++) {
        if (s[i] == '\n') {
            cursor_col = 0;
            cursor_row++;
            continue;
        }
        VGA_MEM[cursor_row * VGA_WIDTH + cursor_col] = vga_entry(s[i]);
        cursor_col++;
        if (cursor_col >= VGA_WIDTH) {
            cursor_col = 0;
            cursor_row++;
        }
    }
}

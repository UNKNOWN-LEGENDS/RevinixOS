#ifndef BOOTINFO_H
#define BOOTINFO_H
#include <stdint.h>

// Framebuffer GRUB set up for us (multiboot2 tag type 8).
struct boot_framebuffer {
    uint64_t addr;          // physical address of pixel (0,0)
    uint32_t pitch;         // bytes per row (can be more than width * bytes/pixel)
    uint32_t width;         // pixels
    uint32_t height;        // pixels
    uint8_t  bpp;           // bits per pixel
    uint8_t  type;          // 0 = indexed, 1 = RGB, 2 = EGA text
    uint8_t  red_pos, red_size;
    uint8_t  green_pos, green_size;
    uint8_t  blue_pos, blue_size;
};

// Copy the multiboot2 tags we need into kernel variables. Must run first in
// kmain: the mb2 info struct sits in RAM the PMM later hands out (and the
// first page table already landed on top of it once).
void bootinfo_init(uint64_t mb2_info);

// The framebuffer GRUB gave us, or 0 (NULL) if none.
const struct boot_framebuffer* bootinfo_framebuffer(void);

#endif
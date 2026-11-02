#include "bootinfo.h"
#include "kprintf.h"

struct __attribute__((packed)) mb2_tag {
    uint32_t type;
    uint32_t size;
};

// Multiboot2 framebuffer info tag (type 8), GRUB's layout.
struct __attribute__((packed)) mb2_tag_framebuffer {
    uint32_t type;          // 8
    uint32_t size;
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t  bpp;
    uint8_t  fb_type;
    uint16_t reserved;
    // for fb_type 1 (RGB): where each colour lives inside a pixel
    uint8_t  red_pos,   red_size;
    uint8_t  green_pos, green_size;
    uint8_t  blue_pos,  blue_size;
};

static struct boot_framebuffer fb;
static int have_fb = 0;

void bootinfo_init(uint64_t mb2_info) {
    uint8_t* base = (uint8_t*)mb2_info;
    uint32_t total_size = *(uint32_t*)base;
    uint8_t* end = base + total_size;
    uint8_t* p = base + 8;                     // skip total_size + reserved

    while (p < end) {
        struct mb2_tag* tag = (struct mb2_tag*)p;
        if (tag->type == 0) break;             // end tag
        if (tag->type == 8) {
            struct mb2_tag_framebuffer* t = (struct mb2_tag_framebuffer*)tag;
            fb.addr   = t->addr;
            fb.pitch  = t->pitch;
            fb.width  = t->width;
            fb.height = t->height;
            fb.bpp    = t->bpp;
            fb.type   = t->fb_type;
            if (t->fb_type == 1) {
                fb.red_pos   = t->red_pos;   fb.red_size   = t->red_size;
                fb.green_pos = t->green_pos; fb.green_size = t->green_size;
                fb.blue_pos  = t->blue_pos;  fb.blue_size  = t->blue_size;
            }
            have_fb = 1;
        }
        p += (tag->size + 7) & ~7u;            // tags are 8-byte aligned
    }

    if (have_fb)
        kprintf("bootinfo: framebuffer %dx%d, %d bpp, type %d, pitch %d, phys %p\n",
                (int)fb.width, (int)fb.height, (int)fb.bpp, (int)fb.type,
                (int)fb.pitch, (void*)fb.addr);
    else
        kprintf("bootinfo: no framebuffer tag (text mode)\n");
}

const struct boot_framebuffer* bootinfo_framebuffer(void) {
    return have_fb ? &fb : 0;
}
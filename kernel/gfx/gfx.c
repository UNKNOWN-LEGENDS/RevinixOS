#include "gfx.h"
#include "font.h"
#include "../bootinfo.h"
#include "../mm/pmm.h"
#include "../mm/vmm.h"
#include "../kprintf.h"

static int       ready = 0;
static int       W, H;            // screen size in pixels
static uint32_t  pitch;           // framebuffer bytes per row
static int       bytespp;         // framebuffer bytes per pixel: 4 or 3
static volatile uint8_t* front;   // the real framebuffer (video memory)
static uint32_t* back;            // back buffer in RAM: W*H pixels, no padding

int gfx_ready(void)  { return ready; }
int gfx_width(void)  { return W; }
int gfx_height(void) { return H; }

int gfx_init(void) {
    const struct boot_framebuffer* fb = bootinfo_framebuffer();
    if (!fb) {
        kprintf("gfx: no framebuffer from GRUB - staying in text mode\n");
        return -1;
    }
    if (fb->type != 1 || (fb->bpp != 32 && fb->bpp != 24)) {
        kprintf("gfx: unsupported framebuffer (type %d, %d bpp) - text mode\n",
                (int)fb->type, (int)fb->bpp);
        return -1;
    }
    if (fb->red_pos != 16 || fb->green_pos != 8 || fb->blue_pos != 0)
        kprintf("gfx: warning: unusual colour layout (R@%d G@%d B@%d)\n",
                (int)fb->red_pos, (int)fb->green_pos, (int)fb->blue_pos);

    W = (int)fb->width;
    H = (int)fb->height;
    pitch = fb->pitch;
    bytespp = fb->bpp / 8;

    // Reach video memory through the HHDM (shared by every address space),
    // not the low identity map (which user processes don't have).
    front = (volatile uint8_t*)vmm_map_mmio(fb->addr, (uint64_t)pitch * H);

    // Back buffer: physically contiguous RAM, used through the HHDM.
    uint64_t bytes  = (uint64_t)W * H * 4;
    uint64_t frames = (bytes + FRAME_SIZE - 1) / FRAME_SIZE;
    uint64_t phys   = pmm_alloc_contig(frames);
    if (!phys) {
        kprintf("gfx: no memory for a %d KB back buffer\n", (int)(bytes / 1024));
        return -1;
    }
    back = (uint32_t*)phys_to_virt(phys);

    ready = 1;
    kprintf("gfx: %dx%d, %d bpp, pitch %d, back buffer %d KB\n",
            W, H, bytespp * 8, (int)pitch, (int)(bytes / 1024));
    return 0;
}

// Clip a rectangle to the screen. Returns 0 if nothing is left.
static int clip(int* x, int* y, int* w, int* h) {
    if (*x < 0) { *w += *x; *x = 0; }
    if (*y < 0) { *h += *y; *y = 0; }
    if (*x + *w > W) *w = W - *x;
    if (*y + *h > H) *h = H - *y;
    return *w > 0 && *h > 0;
}

void gfx_fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (!ready || !clip(&x, &y, &w, &h)) return;
    for (int row = y; row < y + h; row++) {
        uint32_t* p = back + (uint64_t)row * W + x;
        for (int i = 0; i < w; i++) p[i] = color;
    }
}

void gfx_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg) {
    if (!ready) return;
    if (c < FONT_FIRST || c > FONT_LAST) c = '?';
    const uint8_t* glyph = font8x16[c - FONT_FIRST];
    for (int row = 0; row < FONT_H; row++) {
        int py = y + row;
        if (py < 0 || py >= H) continue;
        uint8_t bits = glyph[row];
        for (int col = 0; col < FONT_W; col++) {
            int px = x + col;
            if (px < 0 || px >= W) continue;
            back[(uint64_t)py * W + px] = (bits & (0x80 >> col)) ? fg : bg;
        }
    }
}

void gfx_draw_text(int x, int y, const char* s, uint32_t fg, uint32_t bg) {
    for (int i = 0; s[i]; i++) gfx_draw_char(x + i * FONT_W, y, s[i], fg, bg);
}

void gfx_scroll_up(int x, int y, int w, int h, int dy, uint32_t bg) {
    if (!ready || !clip(&x, &y, &w, &h)) return;
    if (dy >= h) { gfx_fill_rect(x, y, w, h, bg); return; }
    for (int row = y; row < y + h - dy; row++) {
        uint32_t* dst = back + (uint64_t)row * W + x;
        uint32_t* src = back + (uint64_t)(row + dy) * W + x;
        for (int i = 0; i < w; i++) dst[i] = src[i];
    }
    gfx_fill_rect(x, y + h - dy, w, dy, bg);
}

void gfx_present(int x, int y, int w, int h) {
    if (!ready || !clip(&x, &y, &w, &h)) return;
    for (int row = y; row < y + h; row++) {
        uint32_t* src = back + (uint64_t)row * W + x;
        if (bytespp == 4) {
            volatile uint32_t* dst = (volatile uint32_t*)(front + (uint64_t)row * pitch) + x;
            for (int i = 0; i < w; i++) dst[i] = src[i];
        } else {                                  // 24 bpp: 3 bytes, blue first
            volatile uint8_t* dst = front + (uint64_t)row * pitch + (uint64_t)x * 3;
            for (int i = 0; i < w; i++) {
                dst[i * 3 + 0] = (uint8_t)(src[i]);
                dst[i * 3 + 1] = (uint8_t)(src[i] >> 8);
                dst[i * 3 + 2] = (uint8_t)(src[i] >> 16);
            }
        }
    }
}
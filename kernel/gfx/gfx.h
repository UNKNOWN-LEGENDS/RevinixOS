#ifndef GFX_H
#define GFX_H
#include <stdint.h>

// 0x00RRGGBB, the layout of a 32-bpp RGB framebuffer.
#define RGB(r, g, b) (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))

// Set up drawing on the framebuffer GRUB gave us. Returns 0 on success,
// -1 if there is no usable 24/32-bpp RGB framebuffer (we stay in VGA text mode).
int gfx_init(void);
int gfx_ready(void);
int gfx_width(void);
int gfx_height(void);

// Everything below draws into the BACK buffer (normal RAM). Nothing appears
// on screen until gfx_present() copies a rectangle to the framebuffer.
void gfx_fill_rect(int x, int y, int w, int h, uint32_t color);
void gfx_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg);
void gfx_draw_text(int x, int y, const char* s, uint32_t fg, uint32_t bg);

// Move the pixels inside a rectangle up by dy, fill the freed rows with bg.
void gfx_scroll_up(int x, int y, int w, int h, int dy, uint32_t bg);

// Copy a rectangle of the back buffer to the screen.
void gfx_present(int x, int y, int w, int h);

void gfx_cursor_set(int x, int y);

#endif
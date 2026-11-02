#include "fbcon.h"
#include "gfx.h"
#include "font.h"

static int active = 0;
static int ox, oy;           // top-left pixel of the console area
static int cols, rows;       // size in character cells
static int cx, cy;           // cursor cell
static uint32_t fg, bg;

int fbcon_active(void) { return active; }

static void present_cell(int c, int r) {
    gfx_present(ox + c * FONT_W, oy + r * FONT_H, FONT_W, FONT_H);
}

// Underline cursor in the bottom two pixel rows of the current cell.
static void draw_cursor(int on) {
    gfx_fill_rect(ox + cx * FONT_W, oy + cy * FONT_H + FONT_H - 2, FONT_W, 2, on ? fg : bg);
    present_cell(cx, cy);
}

void fbcon_init(int x, int y, int w, int h, uint32_t fore, uint32_t back) {
    ox = x; oy = y;
    cols = w / FONT_W;
    rows = h / FONT_H;
    fg = fore; bg = back;
    cx = cy = 0;
    gfx_fill_rect(ox, oy, cols * FONT_W, rows * FONT_H, bg);
    active = 1;
    draw_cursor(1);
}

// Interrupts off while we touch the console: the keyboard IRQ echoes typed
// characters through kprintf, and it must not land in the middle of a scroll.
static uint64_t irq_save(void) {
    uint64_t f;
    __asm__ volatile ("pushfq; pop %0; cli" : "=r"(f) : : "memory");
    return f;
}
static void irq_restore(uint64_t f) {
    __asm__ volatile ("push %0; popfq" : : "r"(f) : "memory", "cc");
}

static void newline(void) {
    cx = 0;
    cy++;
    if (cy >= rows) {
        gfx_scroll_up(ox, oy, cols * FONT_W, rows * FONT_H, FONT_H, bg);
        gfx_present(ox, oy, cols * FONT_W, rows * FONT_H);
        cy = rows - 1;
    }
}

void fbcon_putc(char c) {
    if (!active) return;
    uint64_t f = irq_save();
    draw_cursor(0);

    if (c == '\n') {
        newline();
    } else if (c == '\r') {
        cx = 0;
    } else if (c == '\b') {
        if (cx > 0) cx--;
    } else if (c == '\t') {
        cx = (cx + 4) & ~3;
        if (cx >= cols) newline();
    } else {
        gfx_draw_char(ox + cx * FONT_W, oy + cy * FONT_H, c, fg, bg);
        present_cell(cx, cy);
        if (++cx >= cols) newline();
    }

    draw_cursor(1);
    irq_restore(f);
}

void fbcon_clear(void) {
    if (!active) return;
    uint64_t f = irq_save();
    gfx_fill_rect(ox, oy, cols * FONT_W, rows * FONT_H, bg);
    gfx_present(ox, oy, cols * FONT_W, rows * FONT_H);
    cx = cy = 0;
    draw_cursor(1);
    irq_restore(f);
}

void fbcon_write_at(const char* s, int row, int col) {
    if (!active || row < 0 || row >= rows) return;
    for (int i = 0; s[i] && col + i < cols; i++) {
        gfx_draw_char(ox + (col + i) * FONT_W, oy + row * FONT_H, s[i], fg, bg);
        present_cell(col + i, row);
    }
}
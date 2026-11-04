#include "desktop.h"
#include "../gfx/gfx.h"
#include "../gfx/fbcon.h"
#include "../gfx/font.h"
#include "../drivers/rtc.h"
#include "../drivers/mouse.h"

#define TASKBAR_H   36
#define TITLEBAR_H  28

#define COL_TASKBAR   RGB(0x14, 0x14, 0x1E)
#define COL_TASKLINE  RGB(0x3A, 0x3A, 0x55)
#define COL_START     RGB(0x2D, 0x6C, 0xDF)
#define COL_TASKBTN   RGB(0x2A, 0x2A, 0x3C)
#define COL_TEXT      RGB(0xF0, 0xF0, 0xF5)
#define COL_TEXT_DIM  RGB(0xB0, 0xB0, 0xC8)
#define COL_SHADOW    RGB(0x0A, 0x0A, 0x14)
#define COL_FRAME     RGB(0x3A, 0x3A, 0x55)
#define COL_TITLE     RGB(0x26, 0x26, 0x3A)
#define COL_CON_BG    RGB(0x1E, 0x1E, 0x2E)
#define COL_CON_FG    RGB(0xE0, 0xE0, 0xE8)

static int W, H;
static uint8_t last_second = 0xFF;

static void put2(char* p, int v) { p[0] = (char)('0' + v / 10); p[1] = (char)('0' + v % 10); }

static void draw_clock(const struct rtc_time* t) {
    static const char* mon[] = { "Jan","Feb","Mar","Apr","May","Jun",
                                 "Jul","Aug","Sep","Oct","Nov","Dec" };
    // "HH:MM:SS  DD Mon YYYY"
    char s[24];
    put2(s + 0, t->hour);   s[2] = ':';
    put2(s + 3, t->minute); s[5] = ':';
    put2(s + 6, t->second);
    s[8] = ' '; s[9] = ' ';
    put2(s + 10, t->day);   s[12] = ' ';
    const char* m = (t->month >= 1 && t->month <= 12) ? mon[t->month - 1] : "???";
    s[13] = m[0]; s[14] = m[1]; s[15] = m[2]; s[16] = ' ';
    put2(s + 17, t->year / 100); put2(s + 19, t->year % 100);
    s[21] = '\0';

    int w = 21 * FONT_W;
    int x = W - w - 14;
    int y = H - TASKBAR_H + (TASKBAR_H - FONT_H) / 2;
    gfx_fill_rect(x - 4, y, w + 8, FONT_H, COL_TASKBAR);
    gfx_draw_text(x, y, s, COL_TEXT, COL_TASKBAR);
    gfx_present(x - 4, y, w + 8, FONT_H);
}

static void draw_wallpaper(void) {
    // Vertical gradient, deep blue at the top to purple at the bottom.
    int h = H - TASKBAR_H;
    for (int y = 0; y < h; y++) {
        int r = 0x18 + (0x5A - 0x18) * y / h;
        int g = 0x20 + (0x1C - 0x20) * y / h;
        int b = 0x6E + (0x8C - 0x6E) * y / h;
        gfx_fill_rect(0, y, W, 1, RGB(r, g, b));
    }
}

static void draw_taskbar(void) {
    int y = H - TASKBAR_H;
    gfx_fill_rect(0, y, W, TASKBAR_H, COL_TASKBAR);
    gfx_fill_rect(0, y, W, 1, COL_TASKLINE);

    // "start" button
    gfx_fill_rect(6, y + 5, 11 * FONT_W + 16, TASKBAR_H - 10, COL_START);
    gfx_draw_text(14, y + (TASKBAR_H - FONT_H) / 2, "RevinixOS", COL_TEXT, COL_START);

    // one task button for the terminal window
    int bx = 6 + 11 * FONT_W + 16 + 8;
    gfx_fill_rect(bx, y + 5, 10 * FONT_W + 16, TASKBAR_H - 10, COL_TASKBTN);
    gfx_fill_rect(bx, y + TASKBAR_H - 7, 10 * FONT_W + 16, 2, COL_START);   // "active" underline
    gfx_draw_text(bx + 8, y + (TASKBAR_H - FONT_H) / 2, "Terminal", COL_TEXT_DIM, COL_TASKBTN);
}

// Draws a window frame + title bar; returns the content rectangle.
static void draw_window(int x, int y, int w, int h, const char* title,
                        int* cx, int* cy, int* cw, int* ch) {
    gfx_fill_rect(x + 6, y + 6, w, h, COL_SHADOW);                 // drop shadow
    gfx_fill_rect(x, y, w, h, COL_FRAME);                          // 1 px frame
    gfx_fill_rect(x + 1, y + 1, w - 2, TITLEBAR_H - 1, COL_TITLE); // title bar
    gfx_draw_text(x + 12, y + (TITLEBAR_H - FONT_H) / 2, title, COL_TEXT, COL_TITLE);

    // three window buttons on the right (drawn only; no mouse yet)
    uint32_t btn[3] = { RGB(0x5A, 0xC8, 0x5A), RGB(0xE6, 0xB4, 0x32), RGB(0xE0, 0x50, 0x50) };
    for (int i = 0; i < 3; i++)
        gfx_fill_rect(x + w - 22 - i * 20, y + 8, 12, 12, btn[2 - i]);

    *cx = x + 1;
    *cy = y + TITLEBAR_H;
    *cw = w - 2;
    *ch = h - TITLEBAR_H - 1;
    gfx_fill_rect(*cx, *cy, *cw, *ch, COL_CON_BG);
}

void desktop_init(void) {
    W = gfx_width();
    H = gfx_height();

    draw_wallpaper();
    draw_taskbar();

    // Terminal window, centred with a margin, above the taskbar.
    int mx = W / 16, my = H / 18;
    int cx, cy, cw, ch;
    draw_window(mx, my, W - 2 * mx, H - TASKBAR_H - 2 * my,
                "Terminal - RevinixOS shell", &cx, &cy, &cw, &ch);

    gfx_present(0, 0, W, H);

    // From here on, kprintf / the shell draw inside the window.
    fbcon_init(cx + 8, cy + 6, cw - 16, ch - 12, COL_CON_FG, COL_CON_BG);

    struct rtc_time t;
    rtc_read(&t);
    last_second = t.second;
    draw_clock(&t);
}

void desktop_tick(void) {
    if (!gfx_ready() || !fbcon_active()) return;
    struct mouse_state m;
    if (mouse_poll(&m))
        gfx_cursor_set(m.x, m.y);
    struct rtc_time t;
    rtc_read(&t);
    if (t.second == last_second) return;
    last_second = t.second;
    draw_clock(&t);
}
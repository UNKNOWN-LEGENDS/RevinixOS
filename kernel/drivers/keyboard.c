#include "keyboard.h"
#include "../kprintf.h"

#define KB_BUF_SIZE 128

// US QWERTY scancode -> ASCII, unshifted. Index by make-code (0x00..0x3A here).
// 0 = no printable char (function/modifier/unmapped).
static const char kbd_map[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8',   // 0x00-0x09
    '9', '0', '-', '=', '\b','\t','q', 'w', 'e', 'r',   // 0x0A-0x13
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,    // 0x14-0x1D (0x1C=Enter,0x1D=Ctrl)
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',   // 0x1E-0x27
    '\'','`', 0,  '\\','z', 'x', 'c', 'v', 'b', 'n',     // 0x28-0x31 (0x2A=LShift)
    'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,      // 0x32-0x3B (0x36=RShift,0x39=Space)
    // rest (function keys etc.) left as 0
};

// Shifted variants for the same indices.
static const char kbd_map_shift[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*',
    '(', ')', '_', '+', '\b','\t','Q', 'W', 'E', 'R',
    'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    '"', '~', 0,  '|', 'Z', 'X', 'C', 'V', 'B', 'N',
    'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,
};

static char     line_buf[KB_BUF_SIZE];
static uint32_t line_len = 0;
static int      shift_down = 0;

// A single completed line, latched on Enter for the poller to pick up.
static char     ready_line[KB_BUF_SIZE];
static int      line_ready = 0;

void keyboard_handle_scancode(uint8_t scancode) {
    // Key release: high bit set. Only shift-release matters; ignore the rest.
    if (scancode & 0x80) {
        uint8_t make = scancode & 0x7F;
        if (make == 0x2A || make == 0x36) shift_down = 0;
        return;
    }

    // Shift press.
    if (scancode == 0x2A || scancode == 0x36) { shift_down = 1; return; }

    if (scancode >= 128) return;
    char c = shift_down ? kbd_map_shift[scancode] : kbd_map[scancode];
    if (c == 0) return;   // unmapped / modifier

    if (c == '\n') {
        kprintf("\n");
        line_buf[line_len] = '\0';
        // latch the completed line for the poller
        for (uint32_t i = 0; i <= line_len; i++) ready_line[i] = line_buf[i];
        line_ready = 1;
        line_len = 0;
        return;
    }

    if (c == '\b') {
        if (line_len > 0) {
            line_len--;
            kprintf("\b \b");   // move back, erase glyph, move back again
        }
        return;
    }

    // Normal printable char: store (if room) and echo.
    if (line_len < KB_BUF_SIZE - 1) {
        line_buf[line_len++] = c;
        char s[2] = { c, '\0' };
        kprintf("%s", s);       // echo so typing is visible
    }
}

int keyboard_poll_line(char* out, uint32_t out_size) {
    __asm__ volatile ("cli");
    if (!line_ready) { __asm__ volatile ("sti"); return 0; }
    uint32_t i = 0;
    for (; ready_line[i] != '\0' && i < out_size - 1; i++) out[i] = ready_line[i];
    out[i] = '\0';
    line_ready = 0;
    __asm__ volatile ("sti");
    return 1;
}
#include "mouse.h"
#include "../arch/x86_64/io.h"
#include "../arch/x86_64/pic.h"
#include "../arch/x86_64/irqflags.h"
#include "../gfx/gfx.h"
#include "../kprintf.h"

// 8042 PS/2 controller ports
#define PS2_DATA    0x60
#define PS2_STATUS  0x64        // read
#define PS2_CMD     0x64        // write

#define ST_OUT_FULL 0x01        // a byte is waiting for us in PS2_DATA
#define ST_IN_FULL  0x02        // controller hasn't taken our last byte yet

#define PS2_TIMEOUT 100000

static volatile int      mx, my;          // position
static volatile uint8_t  mbuttons;
static volatile uint32_t mpackets;
static volatile int      mchanged = 1;    // draw the cursor once at startup
static uint8_t packet[3];
static int     packet_idx = 0;
static int     ready = 0;

static int wait_write(void) {
    for (int i = 0; i < PS2_TIMEOUT; i++)
        if (!(inb(PS2_STATUS) & ST_IN_FULL)) return 0;
    return -1;
}

static int wait_read(void) {
    for (int i = 0; i < PS2_TIMEOUT; i++)
        if (inb(PS2_STATUS) & ST_OUT_FULL) return 0;
    return -1;
}

static int ctrl_cmd(uint8_t cmd) {
    if (wait_write()) return -1;
    outb(PS2_CMD, cmd);
    return 0;
}

static int read_config(uint8_t* cfg) {
    if (ctrl_cmd(0x20) || wait_read()) return -1;     // 0x20 = read config byte
    *cfg = inb(PS2_DATA);
    return 0;
}

static int write_config(uint8_t cfg) {
    if (ctrl_cmd(0x60) || wait_write()) return -1;    // 0x60 = write config byte
    outb(PS2_DATA, cfg);
    return 0;
}

// Send one byte to the mouse (not the keyboard) and wait for its ACK (0xFA).
static int mouse_write(uint8_t b) {
    if (ctrl_cmd(0xD4) || wait_write()) return -1;    // 0xD4 = next byte goes to port 2
    outb(PS2_DATA, b);
    if (wait_read()) return -1;
    return inb(PS2_DATA) == 0xFA ? 0 : -1;
}

static int screen_w(void) { return gfx_ready() ? gfx_width()  : 640; }
static int screen_h(void) { return gfx_ready() ? gfx_height() : 480; }

int mouse_init(void) {
    uint64_t f = irq_save();      // poll the controller ourselves; no IRQ may steal bytes
    uint8_t cfg;
    int ok = 0;

    for (int i = 0; i < 16 && (inb(PS2_STATUS) & ST_OUT_FULL); i++)
        (void)inb(PS2_DATA);      // drop stale bytes

    if (ctrl_cmd(0xA8) == 0 &&                          // 0xA8 = enable port 2 (mouse)
        read_config(&cfg) == 0 &&
        write_config((uint8_t)(cfg & ~0x20)) == 0 &&    // bit 5 clear = port 2 clock on
        mouse_write(0xF6) == 0 &&                       // set defaults
        mouse_write(0xF4) == 0 &&                       // enable data reporting
        read_config(&cfg) == 0 &&
        write_config((uint8_t)(cfg | 0x02)) == 0)       // bit 1 = raise IRQ12 for mouse data
        ok = 1;

    irq_restore(f);

    if (!ok) {
        kprintf("mouse: no PS/2 mouse answered - disabled\n");
        return -1;
    }

    mx = screen_w() / 2;
    my = screen_h() / 2;
    packet_idx = 0;
    ready = 1;
    pic_clear_mask(2);            // IRQ2 = cascade from the slave PIC
    pic_clear_mask(12);           // IRQ12 = PS/2 mouse
    kprintf("mouse: PS/2 mouse enabled (IRQ12)\n");
    return 0;
}

void mouse_handle_irq(void) {
    if (!(inb(PS2_STATUS) & ST_OUT_FULL)) return;
    uint8_t b = inb(PS2_DATA);
    if (!ready) return;

    // Byte 0 always has bit 3 set. If it doesn't, we're out of step: drop
    // bytes until a plausible first byte comes along.
    if (packet_idx == 0 && !(b & 0x08)) return;
    packet[packet_idx++] = b;
    if (packet_idx < 3) return;
    packet_idx = 0;

    if (packet[0] & 0xC0) return;                    // X/Y overflow: discard

    // 9-bit two's complement: the sign bits live in byte 0 (bit 4 = X, bit 5 = Y).
    int dx = (int)packet[1] - ((packet[0] << 4) & 0x100);
    int dy = (int)packet[2] - ((packet[0] << 3) & 0x100);

    int x = mx + dx;
    int y = my - dy;                                  // mouse Y grows upward, screen Y downward
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= screen_w()) x = screen_w() - 1;
    if (y >= screen_h()) y = screen_h() - 1;

    mx = x;
    my = y;
    mbuttons = packet[0] & 0x07;
    mpackets++;
    mchanged = 1;
}

void mouse_get(struct mouse_state* out) {
    uint64_t f = irq_save();
    out->x = mx; out->y = my; out->buttons = mbuttons; out->packets = mpackets;
    irq_restore(f);
}

int mouse_poll(struct mouse_state* out) {
    uint64_t f = irq_save();
    int changed = mchanged;
    mchanged = 0;
    out->x = mx; out->y = my; out->buttons = mbuttons; out->packets = mpackets;
    irq_restore(f);
    return ready && changed;
}
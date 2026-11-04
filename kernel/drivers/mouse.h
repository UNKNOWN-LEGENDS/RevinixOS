#ifndef MOUSE_H
#define MOUSE_H
#include <stdint.h>

#define MOUSE_LEFT   0x01
#define MOUSE_RIGHT  0x02
#define MOUSE_MIDDLE 0x04

struct mouse_state {
    int      x, y;          // pointer position in screen pixels
    uint8_t  buttons;       // MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE
    uint32_t packets;       // packets received since boot (for debugging)
};

// Enable the PS/2 auxiliary port, switch the mouse on, unmask IRQ12.
// Returns 0 on success, -1 if no mouse answered.
int  mouse_init(void);

// Called from the IRQ12 handler: reads one byte, decodes full 3-byte packets.
void mouse_handle_irq(void);

// Copy the current state. Returns 1 if it changed since the last call.
int  mouse_poll(struct mouse_state* out);

// Copy the current state without clearing the "changed" flag.
void mouse_get(struct mouse_state* out);

#endif
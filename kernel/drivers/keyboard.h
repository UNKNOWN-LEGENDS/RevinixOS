#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdint.h>

// Called from the keyboard IRQ (IRQ1) with the raw scancode from port 0x60.
void keyboard_handle_scancode(uint8_t scancode);

// Non-blocking: returns 1 and copies the latest completed line into `out`
// (null-terminated, up to out_size-1 chars) if the user has pressed Enter
// since the last call; returns 0 otherwise.
int keyboard_poll_line(char* out, uint32_t out_size);

#endif
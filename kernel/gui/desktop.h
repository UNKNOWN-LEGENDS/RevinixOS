#ifndef DESKTOP_H
#define DESKTOP_H

// Draw the desktop (wallpaper, taskbar, terminal window) and route the
// console into the terminal window. Needs gfx_init() to have succeeded.
void desktop_init(void);

// Call often (e.g. from the shell's idle loop): redraws the taskbar clock
// whenever the second changes. Does nothing in text mode.
void desktop_tick(void);

#endif
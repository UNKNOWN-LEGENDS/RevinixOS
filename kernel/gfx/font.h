#ifndef GFX_FONT_H
#define GFX_FONT_H
#include <stdint.h>

#define FONT_W      8
#define FONT_H      16
#define FONT_FIRST  0x20                 // first glyph in the table (space)
#define FONT_LAST   0x7E                 // last glyph (~)
#define FONT_GLYPHS (FONT_LAST - FONT_FIRST + 1)

extern const uint8_t font8x16[FONT_GLYPHS][16];

#endif
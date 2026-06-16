#ifndef BITMAP_FONT_H
#define BITMAP_FONT_H

#include <stdint.h>

/* Glyph structure: bitmap stored as bytes, one byte per row */
typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t rows[10];
} Glyph;

/* Render text using bitmap font with Unicode blocks */
void render_bitmap_text(const char *text);

#endif

#ifndef LYRICS_H
#define LYRICS_H

#include <stdio.h>
#include "song_utils.h"
#ifdef _BITMAP_FONT
#include "bitmap_font.h"
#endif

/* normal_mode_print_timestamps: when non-zero, print timestamps in normal mode */
void display_lyrics(str lyrics, str artist, str title, int pretty_mode, int normal_mode_print_timestamps);

#endif /* LYRICS_H */
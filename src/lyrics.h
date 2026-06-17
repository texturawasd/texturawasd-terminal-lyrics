#ifndef LYRICS_H
#define LYRICS_H

#include <stdio.h>
#include "song_utils.h"
#include "bitmap_font.h"
#include "../common_utils/simple_strings.h"

#include "lyrics.c"

void display_lyrics(str lyrics, str artist, str title, int pretty_mode);

#endif /* LYRICS_H */
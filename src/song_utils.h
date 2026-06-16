#ifndef SONG_UTILS_H
#define SONG_UTILS_H

#include "../common_utils/simple_strings.h"
#include "../common_utils/process_utils.h"
#include "song_utils.c"
#include "utils.h"

str get_figlet_tool(void);

str get_current_song(void);

str get_artist(void);

str get_title(void);

double get_current_position(void);

str clean_title(const char *title);

str get_lyrics(const char *artist, const char *title);

str get_current_lyric_line(const char *lyrics, double current_position);

#endif
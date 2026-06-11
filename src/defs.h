#pragma once

#include "../common_utils/string_utils.c"
#include "../common_utils/have.c"
#include "utils.c"

/* determines `figlet` or `toilet`, defaults to `toilet` */
str get_figlet_tool(void);

str get_current_song(void);
str get_artist(void);
str get_title(void);
double get_current_position(void);

typedef struct Word {
    str word;
    double start;
    double end;
} word;

typedef struct {
    bool use_figlet;
    int refresh_rate_ms;
} options;

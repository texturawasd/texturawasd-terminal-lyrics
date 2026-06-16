#ifndef DEFS_H
#define DEFS_H

#include "../common_utils/simple_strings.h"
#include "../common_utils/have.h"

/* determines `figlet` or `toilet`, defaults to `toilet` */
str get_figlet_tool(void);

str get_current_song(void);
str get_artist(void);
str get_title(void);
double get_current_position(void);
str get_lyrics(const char *artist, const char *title);
str clean_title(const char *title);
char *extract_json_string(char *start);
void debug_all_metadata(void);

#ifdef _OPTS
void do_options(int argc, char **argv);
int should_run_firefox_bridge_server(void);
#endif

#ifdef _FIREFOX_EXTENSION_BRIDGE
/* Firefox bridge getter functions */
str firefox_get_artist(void);
str firefox_get_title(void);
double firefox_get_position(void);
int firefox_bridge_server(void);
#endif

typedef struct Word {
    str word;
    double start;
    double end;
} word;

typedef struct {
    bool use_figlet;
    int refresh_rate_ms;
} options;

#endif
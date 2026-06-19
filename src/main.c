#if !defined(__linux__) && !defined(_DO_AS_I_SAY)
#error This program is only supported on Linux.
#endif

#include <stdio.h>
#include <curl/curl.h>
#include <jansson.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "song_utils.h"
#include "lyrics.h"

#ifdef _FIREFOX_EXTENSION_BRIDGE_SERVER
#include "thing_for_firefox_extension_bridge_thingy.h"
#endif

#if defined(_OPTS)
#include "options.h"
#endif
#include "defs.h"

int main(int argc, char **argv) {
    /* Set cache directory */
    str cache_dir;
    int pretty_mode = 0;

    #ifdef _OPTS
    pretty_mode = arg_is_present("pretty", argc, &*argv);

    cache_dir = arg_is_present("cache_dir", argc, &*argv) ? get_arg_value("cache_dir", argc, &*argv) : NULL_STRING;
    #ifdef _DEBUG
    printf("DEBUG: pretty_mode: %d\n", pretty_mode);
    #endif
    /* Fallback to default if not provided */
    if (!cache_dir.data || cache_dir.len == 0) {
        cache_dir = expand_home("~/.cache/texturawasd-terminal-lyrics");
    }
    #else
    cache_dir = expand_home("~/.cache/texturawasd-terminal-lyrics");
    #endif

    dir_exists(cache_dir.data) || mkdir(cache_dir.data, 0755);

    #if !defined(_OPTS)
    (void)argc;
    (void)argv;
    #endif

    #ifdef _FIREFOX_EXTENSION_BRIDGE_SERVER
    /* Build and run the Firefox bridge server only */
    return firefox_bridge_server();
    #endif

    /* Normal client mode (with optional Firefox bridge support) */

    #ifdef _FIREFOX_EXTENSION_BRIDGE
    /* Give extension a brief moment to start sending data */
    usleep(50000);  /* 50ms */
    #endif

    #ifdef _DEBUG
    debug_all_metadata();
    #endif
    str artist = get_artist();
    str title = get_title();
    double position = get_current_position();

    if (artist.data) {
        str_rtrim(&artist);
    }
    if (title.data) {
        str_rtrim(&title);
    }

    #ifdef _DEBUG
    /* Debug: print what we got */
    fprintf(stderr, "DEBUG artist.data: '%s', artist.len: %zu\n", artist.data ? artist.data : "(null)", artist.len);
    fprintf(stderr, "DEBUG title.data: '%s', title.len: %zu\n", title.data ? title.data : "(null)", title.len);
    #endif

    /* Display artist - title, or just title if artist is empty */
    if (artist.data && artist.len > 0) {
        printf("%s - %s\n", artist.data, title.data ? title.data : "");
    } else if (title.data && title.len > 0) {
        printf("%s\n", title.data);
    }

    printf("%.2f sec\n", position);

    /* Build cache filename for potential cached lyrics */
    char cache_file[512] = {0};
    if (cache_dir.data && cache_dir.len > 0 && artist.data && artist.len > 0 && title.data && title.len > 0) {
        /* Sanitize artist and title for filename (replace / with _) */
        char artist_safe[256] = {0};
        char title_safe[256] = {0};

        for (size_t i = 0; i < artist.len && i < sizeof(artist_safe) - 1; i++) {
            artist_safe[i] = (artist.data[i] == '/') ? '_' : artist.data[i];
        }
        for (size_t i = 0; i < title.len && i < sizeof(title_safe) - 1; i++) {
            title_safe[i] = (title.data[i] == '/') ? '_' : title.data[i];
        }

        snprintf(cache_file, sizeof(cache_file), "%s/%s - %s.txt", cache_dir.data, artist_safe, title_safe);

        #ifdef _DEBUG
        printf("DEBUG: Cache file path: '%s'\n", cache_file);
        #endif
    }

    /* Try to load from cache first */
    str lyrics = NULL_STRING;
    if (cache_file[0] != '\0' && file_exists(cache_file)) {
        #ifdef _DEBUG
        printf("DEBUG: Loading lyrics from cache: %s\n", cache_file);
        #endif
        lyrics = read_entire_file(cache_file);
    }

    /* If not in cache, fetch from API */
    if (!lyrics.data || lyrics.len == 0) {
        lyrics = get_lyrics(artist.data ? artist.data : "", title.data ? title.data : "");

        /* Cache the newly fetched lyrics */
        if (lyrics.data && lyrics.len > 0 && cache_file[0] != '\0') {
            #ifdef _DEBUG
            printf("DEBUG: Caching lyrics to: %s\n", cache_file);
            #endif

            /* Ensure cache directory exists */
            if (mkdir(cache_dir.data, 0755) < 0) {
                #ifdef _DEBUG
                perror("DEBUG: mkdir failed");
                #endif
            }
            write_entire_file(cache_file, lyrics.data);
        }
    }

    /* Display lyrics */

    display_lyrics(lyrics, artist, title, pretty_mode);

    if (artist.data) str_destroy(&artist);
    if (title.data) str_destroy(&title);

    return 0;
}
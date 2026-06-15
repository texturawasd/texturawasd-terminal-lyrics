#if !defined(__linux__) && !defined(_DO_AS_I_SAY)
#error This program is only supported on Linux.
#endif


#include <stdio.h>
#include <curl/curl.h>
#include <jansson.h>
#include <unistd.h>

#include "../common_utils/string_utils.c"

#ifdef _FIREFOX_EXTENSION_BRIDGE_SERVER
#include "thing_for_firefox_extension_bridge_thingy.c"
int firefox_bridge_server(void);
#endif

#if defined(_OPTS)
#include "options.c"
#endif
#include "defs.h"

int main(int argc, char **argv) {
    #if !defined (_OPTS)
    (void)argc;
    (void)argv;
    #endif
    #ifdef _FIREFOX_EXTENSION_BRIDGE_SERVER
    /* Build and run the Firefox bridge server only */
    return firefox_bridge_server();
    #endif

    /* Normal client mode (with optional Firefox bridge support) */

    #ifdef _FIREFOX_EXTENSION_BRIDGE
    /* Wait a moment for Firefox extension to send metadata via POST request */
    usleep(200000);  /* 200ms */
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

    /* Fetch and display lyrics */
    str lyrics = get_lyrics(artist.data ? artist.data : "", title.data ? title.data : "");
    if (lyrics.data && lyrics.len > 0) {
        printf("\n%s\n", lyrics.data);
        str_destroy(&lyrics);
    }

    if (artist.data) str_destroy(&artist);
    if (title.data) str_destroy(&title);

    return 0;
}
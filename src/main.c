#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

#include "../common_utils/string_utils.c"

#include "options.c"
#include "defs.h"

int main(int argc, char **argv) {
    do_options(argc, argv);

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

    if (artist.data) str_destroy(&artist);
    if (title.data) str_destroy(&title);

    return 0;
}
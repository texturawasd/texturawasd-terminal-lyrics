#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../common_utils/string_utils.c"
#include "../common_utils/have.c"
#include "defs.h"

#ifdef _FIREFOX_EXTENSION_BRIDGE
#include "thing_for_firefox_extension_bridge_thingy.c"
#endif

str get_figlet_tool(void) {

    str figlet_tool = str_create("toilet"); /* default to toilet */
    if (command_exists("figlet")) {
        figlet_tool.data = "figlet";
    } else if (command_exists("toilet")) {
        figlet_tool.data = "toilet";
    }
    return figlet_tool;
}


#ifdef _DEBUG
/* Debug: get all available metadata */
void debug_all_metadata(void) {

    fprintf(stderr, "DEBUG: All available metadata:\n");
    char *all_meta = capture_output("playerctl metadata");
    if (all_meta) {
        fprintf(stderr, "%s\n", all_meta);
        free(all_meta);
    }

}
#endif


/* Extract a JSON string value, handling escape sequences */
char *extract_json_string(char *start) {
    char *p = start;
    size_t len = 0;

    /* Count characters until closing unescaped quote */
    while (*p) {
        if (*p == '\\' && *(p + 1)) {
            len += 2;  /* skip escape sequence */
            p += 2;
        } else if (*p == '"') {
            break;
        } else {
            len++;
            p++;
        }
    }

    /* Allocate and copy the string */
    char *result = malloc(len + 1);
    if (!result) return NULL;

    char *src = start;
    char *dst = result;

    /* Copy while processing escape sequences */
    while (src < p) {
        if (*src == '\\' && *(src + 1)) {
            char next = *(src + 1);
            switch (next) {
                case 'n': *dst = '\n'; break;
                case 't': *dst = '\t'; break;
                case 'r': *dst = '\r'; break;
                case '"': *dst = '"'; break;
                case '\\': *dst = '\\'; break;
                default: *dst = next; break;
            }
            src += 2;
            dst++;
        } else {
            *dst = *src;
            src++;
            dst++;
        }
    }
    *dst = '\0';

    return result;
}

/* URL encode a string for use in query parameters */
str url_encode(str s) {
    if (!s.data) return str_create("");

    str encoded = {0};
    encoded.cap = s.len * 3 + 1;
    encoded.data = malloc(encoded.cap);
    if (!encoded.data) return encoded;

    size_t pos = 0;
    for (const char *p = s.data; *p; p++) {
        if (isalnum((unsigned char)*p) || *p == '-' || *p == '_' || *p == '.' || *p == '~') {
            encoded.data[pos++] = *p;
        } else {
            pos += sprintf(encoded.data + pos, "%%%02X", (unsigned char)*p);
        }
    }
    encoded.data[pos] = '\0';
    encoded.len = pos;
    return encoded;
}
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../common_utils/string_utils.c"
#include "../common_utils/have.c"
#include "../common_utils/process_utils.c"
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

/* Get currently playing song metadata from playerctl */
str get_current_song(void) {
    #ifdef _FIREFOX_EXTENSION_BRIDGE
    str firefox_artist = firefox_get_artist();
    str firefox_title = firefox_get_title();
    if (firefox_artist.len > 0 && firefox_title.len > 0) {
        str result = str_create("");
        result.cap = firefox_artist.len + firefox_title.len + 5;
        result.data = malloc(result.cap);
        snprintf(result.data, result.cap, "%s - %s", firefox_artist.data, firefox_title.data);
        result.len = strlen(result.data);
        str_destroy(&firefox_artist);
        str_destroy(&firefox_title);
        return result;
    }
    str_destroy(&firefox_artist);
    str_destroy(&firefox_title);
    #endif
    return str_create(capture_output("playerctl metadata --format '{{artist}} - {{title}}'"));
}

/* Get artist from playerctl or Firefox bridge */
str get_artist(void) {
    #ifdef _FIREFOX_EXTENSION_BRIDGE
    str firefox_artist = firefox_get_artist();
    #ifdef _DEBUG
    fprintf(stderr, "DEBUG: get_artist - firefox returned: '%s' (len %zu)\n",
        firefox_artist.data ? firefox_artist.data : "(null)", firefox_artist.len);
    #endif
    if (firefox_artist.len > 0) {
        return firefox_artist;
    }
    str_destroy(&firefox_artist);
    #endif
    return str_create(capture_output("playerctl metadata --format '{{artist}}'"));
}

/* Get title from playerctl or Firefox bridge */
str get_title(void) {
    #ifdef _FIREFOX_EXTENSION_BRIDGE
    str firefox_title = firefox_get_title();
    #ifdef _DEBUG
    fprintf(stderr, "DEBUG: get_title - firefox returned: '%s' (len %zu)\n",
        firefox_title.data ? firefox_title.data : "(null)", firefox_title.len);
    #endif
    if (firefox_title.len > 0) {
        return firefox_title;
    }
    str_destroy(&firefox_title);
    #endif
    return str_create(capture_output("playerctl metadata --format '{{title}}'"));
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

/* Get current position in seconds from playerctl or Firefox bridge */
double get_current_position(void) {
    #ifdef _FIREFOX_EXTENSION_BRIDGE
    double firefox_pos = firefox_get_position();
    if (firefox_pos > 0.0) {
        return firefox_pos;
    }
    #endif
    str pos_str = str_create(capture_output("playerctl position"));
    if (!pos_str.data) {
        return 0.0;
    }
    double pos = strtod(pos_str.data, NULL);
    str_destroy(&pos_str);
    return pos;
}

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

/* Strip common player suffixes from title */
str clean_title(const char *title) {
    if (!title) return str_create("");

    str cleaned = str_create(title);

    /* Remove " | YouTube Music", " - YouTube Music", etc. */
    char *suffixes[] = {
        " | YouTube Music",
        " - YouTube Music",
        " - Spotify",
        " [Audio]",
        NULL
    };

    for (int i = 0; suffixes[i]; i++) {
        char *pos = strstr(cleaned.data, suffixes[i]);
        if (pos) {
            size_t new_len = pos - cleaned.data;
            cleaned.data[new_len] = '\0';
            cleaned.len = new_len;
            break;
        }
    }

    return cleaned;
}

/* Fetch lyrics from lrclib API using search endpoint */
str get_lyrics(const char *artist, const char *title) {
    if (!title || !title[0]) {
        #ifdef _DEBUG
        fprintf(stderr, "DEBUG: Missing title - cannot fetch lyrics\n");
        #endif
        return str_create("");
    }

    /* If we have artist, use the direct endpoint; otherwise use search */
    str lyrics = str_create("");

    if (artist && artist[0]) {
        /* Use direct endpoint with artist */
        str encoded_artist = url_encode(str_create(artist));
        str encoded_title = url_encode(str_create(title));

        char url[2048];
        snprintf(url, sizeof(url),
                 "curl -s 'https://lrclib.net/api/get?artist_name=%s&track_name=%s'",
                 encoded_artist.data, encoded_title.data);

        #ifdef _DEBUG
        fprintf(stderr, "DEBUG: API URL (direct): %s\n", url);
        #endif

        str_destroy(&encoded_artist);
        str_destroy(&encoded_title);

        char *response = capture_output(url);
        if (response) {
            #ifdef _DEBUG
            fprintf(stderr, "DEBUG: API Response: %s\n", response);
            #endif

            char *lyrics_start = strstr(response, "\"syncedLyrics\":\"");
            if (lyrics_start) {
                lyrics_start += 16;  /* skip past "syncedLyrics":" */
                char *extracted = extract_json_string(lyrics_start);
                if (extracted) {
                    lyrics.data = extracted;
                    lyrics.len = strlen(extracted);
                    lyrics.cap = lyrics.len + 1;
                }
            }
            free(response);
        }
    } else {
        /* Use search endpoint for title-only search */
        str clean = clean_title(title);

        /* Don't search if title is just generic player info */
        if (strcmp(clean.data, "YouTube Music") == 0 ||
            strcmp(clean.data, "Spotify") == 0 ||
            strlen(clean.data) < 3) {
            #ifdef _DEBUG
            fprintf(stderr, "DEBUG: Title too generic or short, skipping search\n");
            #endif
            str_destroy(&clean);
            return lyrics;
        }

        str encoded_title = url_encode(clean);

        char url[2048];
        snprintf(url, sizeof(url),
                 "curl -s --max-time 5 'https://lrclib.net/api/search?q=%s'",
                 encoded_title.data);

        #ifdef _DEBUG
        fprintf(stderr, "DEBUG: API URL (search): %s\n", url);
        #endif

        str_destroy(&encoded_title);
        str_destroy(&clean);

        char *response = capture_output(url);
        if (response) {
            #ifdef _DEBUG
            fprintf(stderr, "DEBUG: API Response: %s\n", response);
            #endif

            /* Search returns an array; get first result's lyrics */
            char *lyrics_start = strstr(response, "\"syncedLyrics\":\"");
            if (lyrics_start) {
                lyrics_start += 16;  /* skip past "syncedLyrics":" */
                char *extracted = extract_json_string(lyrics_start);
                if (extracted) {
                    lyrics.data = extracted;
                    lyrics.len = strlen(extracted);
                    lyrics.cap = lyrics.len + 1;
                }
            } else {
                #ifdef _DEBUG
                fprintf(stderr, "DEBUG: No lyrics field found in JSON\n");
                #endif
            }
            free(response);
        }
    }

    return lyrics;
}
#include "../common_utils/string_utils.c"
#include "../common_utils/process_utils.c"
#include "utils.h"

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

/* Parse timestamp in format [mm:ss.cs] and return time in seconds */
static double parse_timestamp(const char *timestamp) {
    if (!timestamp || timestamp[0] != '[') {
        return -1.0;
    }

    int minutes = 0, seconds = 0, centiseconds = 0;
    int parsed = sscanf(timestamp, "[%d:%d.%d]", &minutes, &seconds, &centiseconds);

    if (parsed != 3) {
        return -1.0;
    }

    return minutes * 60.0 + seconds + centiseconds / 100.0;
}

/* Extract text after timestamp from a lyric line */
static str extract_lyric_text(const char *line) {
    if (!line) return str_create("");

    /* Find closing bracket */
    const char *bracket_end = strchr(line, ']');
    if (!bracket_end) return str_create("");

    /* Text starts after the bracket */
    const char *text_start = bracket_end + 1;

    /* Skip leading spaces */
    while (*text_start && isspace((unsigned char)*text_start)) {
        text_start++;
    }

    return str_create(text_start);
}

/* Get the current lyric line based on playback position */
/* Returns the line text (without timestamp) for the current time */
str get_current_lyric_line(const char *lyrics, double current_position) {
    if (!lyrics || lyrics[0] == '\0' || current_position < 0.0) {
        return str_create("");
    }

    /* Split lyrics into lines */
    str lyrics_str = str_create(lyrics);
    char *lyrics_copy = malloc(lyrics_str.len + 1);
    if (!lyrics_copy) {
        str_destroy(&lyrics_str);
        return str_create("");
    }
    strcpy(lyrics_copy, lyrics_str.data);
    str_destroy(&lyrics_str);

    str current_line = str_create("");
    double current_line_time = -1.0;

    /* Parse line by line */
    char *line = strtok(lyrics_copy, "\n");
    while (line) {
        double timestamp = parse_timestamp(line);

        if (timestamp >= 0.0) {
            /* Check if current position falls within this line's time */
            if (timestamp <= current_position) {
                /* Update current line */
                if (current_line.data) {
                    str_destroy(&current_line);
                }
                current_line = extract_lyric_text(line);
                current_line_time = timestamp;
            } else {
                /* We've passed the current time, stop searching */
                break;
            }
        }

        line = strtok(NULL, "\n");
    }

    free(lyrics_copy);
    return current_line;
}
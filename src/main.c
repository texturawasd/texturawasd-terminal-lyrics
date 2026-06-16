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

#include "../common_utils/string_utils.c"
#include "../common_utils/path_utils.c"
#include "../common_utils/file_utils.c"

#if defined(_OPTS)
#include "../common_utils/args.c"
#endif

#include "song_utils.h"

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
    str figlet_tool = NULL_STRING;

    #ifdef _OPTS
    pretty_mode = arg_is_present("pretty", argc, &*argv);
    if (pretty_mode) {
        figlet_tool = get_figlet_tool();
    }

    cache_dir = arg_is_present("cache_dir", argc, &*argv) ? get_arg_value("cache_dir", argc, &*argv) : NULL_STRING;
    #ifdef _DEBUG
    printf("DEBUG: cache_dir from args: '%s'\n", cache_dir.data ? cache_dir.data : "(null)");
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

    restart:
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
    if (lyrics.data && lyrics.len > 0) {
        #if defined(_DEBUG) || defined(_JUST_SHOW_ALL_LYRICS)
        /* Debug mode or explicit flag: show all lyrics */
        printf("\n%s\n", lyrics.data);
        #else
        /* Normal mode: continuously update current lyric line based on position */
        printf("\n");
        fflush(stdout);

        str prev_line = str_create("");
        int update_check = 0;

        while (1) {
            /* Get current position */
            position = get_current_position();

            /* Get current lyric line */
            str current_line = get_current_lyric_line(lyrics.data, position);

            /* Only print if line changed */
            if (!current_line.data || !prev_line.data ||
                strcmp(current_line.data ? current_line.data : "",
                       prev_line.data ? prev_line.data : "") != 0) {

                if (pretty_mode && figlet_tool.data && figlet_tool.len > 0) {
                    /* Pretty mode: use figlet/toilet to display lyric */
                    printf("\033[2J\033[H");  /* Clear screen and move cursor to top */

                    if (current_line.data && current_line.len > 0) {
                        /* Create temp file to safely pass text to figlet */
                        FILE *tmp = fopen("/tmp/lyrics_display.txt", "w");
                        if (tmp) {
                            fprintf(tmp, "%s", current_line.data);
                            fclose(tmp);

                            char cmd[512];
                            /* Use slant font - readable and UTF-8 compatible */
                            snprintf(cmd, sizeof(cmd), "cat /tmp/lyrics_display.txt | %s -f slant -w 200 2>/dev/null || cat /tmp/lyrics_display.txt",
                                     figlet_tool.data);
                            char *output = capture_output(cmd);
                            if (output) {
                                /* Get terminal dimensions */
                                int term_width = 80, term_height = 24;
                                char *width_str = capture_output("tput cols 2>/dev/null");
                                char *height_str = capture_output("tput lines 2>/dev/null");
                                if (width_str) {
                                    term_width = atoi(width_str);
                                    free(width_str);
                                    if (term_width < 40) term_width = 80;
                                }
                                if (height_str) {
                                    term_height = atoi(height_str);
                                    free(height_str);
                                    if (term_height < 10) term_height = 24;
                                }

                                /* Count lines in output */
                                int output_lines = 0;
                                char *output_copy = malloc(strlen(output) + 1);
                                strcpy(output_copy, output);
                                char *line = strtok(output_copy, "\n");
                                while (line) {
                                    output_lines++;
                                    line = strtok(NULL, "\n");
                                }
                                free(output_copy);

                                /* Calculate vertical padding */
                                int vert_padding = (term_height - output_lines) / 2;
                                if (vert_padding < 0) vert_padding = 0;

                                /* Print blank lines for vertical centering */
                                for (int i = 0; i < vert_padding; i++) printf("\n");

                                /* Print each line centered horizontally */
                                output_copy = malloc(strlen(output) + 1);
                                strcpy(output_copy, output);
                                line = strtok(output_copy, "\n");
                                while (line) {
                                    size_t line_len = strlen(line);
                                    if (line_len > 0) {
                                        int horiz_padding = (term_width - (int)line_len) / 2;
                                        if (horiz_padding < 0) horiz_padding = 0;
                                        for (int i = 0; i < horiz_padding; i++) printf(" ");
                                    }
                                    printf("%s\n", line);
                                    line = strtok(NULL, "\n");
                                }
                                free(output_copy);
                                free(output);
                            }
                        }
                    }
                    fflush(stdout);
                } else {
                    /* Normal mode: print with timestamp */
                    if (current_line.data && current_line.len > 0) {
                        printf("[%.2f sec]  %s\n", position, current_line.data);
                    } else {
                        printf("[%.2f sec]\n", position);
                    }
                    fflush(stdout);
                }

                /* Update previous line tracker */
                if (prev_line.data) str_destroy(&prev_line);
                prev_line = str_create(current_line.data ? current_line.data : "");
            }

            if (current_line.data) {
                str_destroy(&current_line);
            }

            /* Check for song change every 10 updates (~1 second with default sleep) */
            update_check++;
            if (update_check >= 10) {
                update_check = 0;
                str new_artist = get_artist();
                str new_title = get_title();

                /* If song changed, break loop to reload */
                if ((new_artist.data && !artist.data) || (!new_artist.data && artist.data) ||
                    (new_artist.data && artist.data && strcmp(new_artist.data, artist.data) != 0) ||
                    (new_title.data && !title.data) || (!new_title.data && title.data) ||
                    (new_title.data && title.data && strcmp(new_title.data, title.data) != 0)) {
                    #ifdef _DEBUG
                    fprintf(stderr, "DEBUG: Song changed, restarting...\n");
                    #endif
                    if (artist.data) str_destroy(&artist);
                    if (title.data) str_destroy(&title);
                    str_destroy(&new_artist);
                    str_destroy(&new_title);
                    str_destroy(&prev_line);
                    break;
                }

                if (new_artist.data) str_destroy(&new_artist);
                if (new_title.data) str_destroy(&new_title);
            }

            /* Update every 100ms */
            usleep(100000);
        }

        if (prev_line.data) str_destroy(&prev_line);
        str_destroy(&lyrics);

        /* Restart the program to get new song's lyrics */
        goto restart;
        #endif
    }

    if (artist.data) str_destroy(&artist);
    if (title.data) str_destroy(&title);

    return 0;
}
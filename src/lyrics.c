#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "song_utils.h"
#include "bitmap_font.h"
#include "../common_utils/simple_strings.h"


/* Helper function to wrap text to terminal width */
static void print_wrapped_text(const char *prefix, const char *text) {
    /* Get terminal width */
    int term_width = 80;
    char *width_str = capture_output("tput cols 2>/dev/null");
    if (width_str) {
        int w = atoi(width_str);
        if (w > 40) term_width = w;
        free(width_str);
    }

    if (!text || *text == '\0') {
        printf("%s\n", prefix);
        return;
    }

    int prefix_len = strlen(prefix);
    int max_line_width = term_width;
    int first_line = 1;

    /* Print initial prefix on first line */
    printf("%s", prefix);
    int col = prefix_len;

    /* Print text word by word */
    const char *p = text;
    while (*p) {
        /* Skip spaces */
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        /* Find end of word */
        const char *word_start = p;
        while (*p && !isspace((unsigned char)*p)) p++;
        int word_len = p - word_start;

        /* Check if word fits on current line */
        int space_before = (col > prefix_len && col > 0) ? 1 : 0;
        if (col == prefix_len) space_before = 0;  /* No space after prefix */

        if (col > prefix_len && col + space_before + word_len > max_line_width) {
            /* Word doesn't fit, wrap to next line */
            printf("\n");
            col = 0;
            first_line = 0;
            space_before = 0;
        }

        /* Add space before word if needed */
        if (col > 0 && space_before) {
            printf(" ");
            col++;
        }

        /* Print word */
        printf("%.*s", word_len, word_start);
        col += word_len;
    }

    printf("\n");
}

void display_lyrics(str lyrics, str artist, str title, int pretty_mode) {
    restart:;
    double position = 0.0;
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

                if (pretty_mode) {
                    /* Pretty mode: use bitmap font with Unicode blocks */
                    printf("\033[2J\033[H");  /* Clear screen and move cursor to top */

                    if (current_line.data && current_line.len > 0) {
                        /* Get terminal dimensions */
                        int term_height = 24;
                        int term_width = 80;

                        char *height_str = capture_output("tput lines 2>/dev/null");
                        if (height_str) {
                            int h = atoi(height_str);
                            if (h > 10) term_height = h;
                            free(height_str);
                        }

                        char *width_str = capture_output("tput cols 2>/dev/null");
                        if (width_str) {
                            int w = atoi(width_str);
                            if (w > 40) term_width = w;
                            free(width_str);
                        }

                        /* Estimate chars per line in bitmap font (roughly term_width / 6) */
                        int chars_per_line = (term_width / 6);
                        if (chars_per_line < 8) chars_per_line = 8;

                        /* First pass: count how many lines we'll need */
                        int num_lines_needed = 1;
                        int line_pos = 0;
                        const char *p = current_line.data;

                        while (*p) {
                            while (*p && !isspace((unsigned char)*p) && line_pos < chars_per_line) {
                                line_pos++;
                                p++;
                            }
                            while (*p && isspace((unsigned char)*p)) p++;
                            if (*p) {
                                const char *next_word_end = p;
                                while (*next_word_end && !isspace((unsigned char)*next_word_end)) next_word_end++;
                                if (line_pos + 1 + (next_word_end - p) > chars_per_line) {
                                    num_lines_needed++;
                                    line_pos = 0;
                                    if (num_lines_needed >= 3) break;  /* Max 3 lines */
                                }
                            }
                        }

                        /* Bitmap font is 7 rows high per line, account for newlines */
                        int bitmap_height = (num_lines_needed * 7) + (num_lines_needed - 1);
                        int total_padding = term_height - bitmap_height;
                        int top_padding = total_padding / 2;
                        int bottom_padding = total_padding - top_padding - 3;  /* Adjust for extra spacing */

                        /* Print blank lines above for vertical centering */
                        for (int i = 0; i < top_padding; i++) printf("\n");

                        /* Second pass: actually render the lines */
                        char line_buf[512] = {0};
                        line_pos = 0;
                        p = current_line.data;
                        int lines_rendered = 0;

                        while (*p && lines_rendered < 3) {
                            /* Fill current line */
                            while (*p && !isspace((unsigned char)*p) && line_pos < chars_per_line) {
                                line_buf[line_pos++] = *p++;
                            }

                            /* Skip spaces */
                            while (*p && isspace((unsigned char)*p)) p++;

                            /* Check if next word fits */
                            if (*p) {
                                const char *next_word_end = p;
                                while (*next_word_end && !isspace((unsigned char)*next_word_end)) next_word_end++;

                                if (line_pos + 1 + (next_word_end - p) > chars_per_line) {
                                    /* Doesn't fit, render and start new line */
                                    line_buf[line_pos] = '\0';
                                    render_bitmap_text(line_buf);
                                    lines_rendered++;
                                    printf("\n");
                                    line_pos = 0;
                                    memset(line_buf, 0, sizeof(line_buf));
                                } else if (line_pos > 0) {
                                    /* Add space and continue */
                                    line_buf[line_pos++] = ' ';
                                }
                            }
                        }

                        /* Render final line */
                        if (line_pos > 0) {
                            line_buf[line_pos] = '\0';
                            render_bitmap_text(line_buf);
                        }

                        /* Print blank lines below for vertical centering */
                        for (int i = 0; i < bottom_padding; i++) printf("\n");

                    }
                    fflush(stdout);
                } else {
                    /* Normal mode: print with timestamp and wrap long lines */
                    if (current_line.data && current_line.len > 0) {
                        char timestamp[32];
                        snprintf(timestamp, sizeof(timestamp), "[%.2f sec]  ", position);

                        /* Get terminal width */
                        int term_width = 80;
                        char *width_str = capture_output("tput cols 2>/dev/null");
                        if (width_str) {
                            int w = atoi(width_str);
                            if (w > 40) term_width = w;
                            free(width_str);
                        }

                        int timestamp_len = strlen(timestamp);

                        /* If text fits on one line, just print it */
                        if (timestamp_len + (int)current_line.len < term_width) {
                            printf("%s%s\n", timestamp, current_line.data);
                        } else {
                            /* Text too long, wrap it */
                            print_wrapped_text(timestamp, current_line.data);
                        }
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
}
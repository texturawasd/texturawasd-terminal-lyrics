#pragma once

#include "../common_utils/string_utils.c"
#include "../common_utils/have.c"
#include "../common_utils/process_utils.c"
#include "defs.h"

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
    return str_create(capture_output("playerctl metadata --format '{{artist}} - {{title}}'"));
}

/* Get artist from playerctl */
str get_artist(void) {
    return str_create(capture_output("playerctl metadata --format '{{artist}}'"));
}

/* Get title from playerctl */
str get_title(void) {
    return str_create(capture_output("playerctl metadata --format '{{title}}'"));
}

/* Get current position in seconds from playerctl */
double get_current_position(void) {
    str pos_str = str_create(capture_output("playerctl position"));
    if (!pos_str.data) {
        return 0.0;
    }
    double pos = strtod(pos_str.data, NULL);
    str_destroy(&pos_str);
    return pos;
}
#include <stdio.h>
#include <curl/curl.h>
#include <jansson.h>

#include "options.c"
#include "defs.h"

int main(int argc, char **argv) {
    do_options(argc, argv);
    puts("texturawasd-terminal-lyrics");
}
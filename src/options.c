#include <stdio.h>
#include "../common_utils/args.h"

void do_options(int argc, char **argv) {
    if (argc == 1) {
        return;
    }
    if (arg_is_present("help", argc, &*argv) || arg_is_present("h", argc, &*argv)) {
        puts("help asked (unimplemented)"); /* help not yet implemented */
    }
}


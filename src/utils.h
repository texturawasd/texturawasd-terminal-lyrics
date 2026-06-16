#ifndef UTILS_H
#define UTILS_H

#include "utils.c"

str get_figlet_tool(void);
#ifdef _DEBUG
void debug_all_metadata(void);
#endif
char *extract_json_string(char *start);
str url_encode(str s);

#endif
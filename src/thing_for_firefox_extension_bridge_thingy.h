#ifndef _FIREFOX_EXTENSION_BRIDGE_INCLUDED
#define _FIREFOX_EXTENSION_BRIDGE_INCLUDED
#ifdef _FIREFOX_EXTENSION_BRIDGE

#include "../common_utils/string_utils.c"

#define FIREFOX_BRIDGE_PORT 3847
#define BUFFER_SIZE 8192

/* Store the last received metadata from Firefox */
typedef struct {
    char *title;
    char *artist;
    char *album;
    char *url;
    double position;
} firefox_metadata;

static char* firefox_extract_json_value(const char *json, const char *key);

static void firefox_store_metadata(const char *title, const char *artist,
                                   const char *album, const char *url, double position);

static void send_http_response(int client, const char *body, const char *content_type);

static void send_cors_preflight(int client);

static void firefox_get_metadata_json(char *buffer, size_t size);

static void firefox_handle_request(int client, const char *request);

int firefox_bridge_server(void);

static char* firefox_http_get_metadata(void);

str firefox_get_artist(void);

str firefox_get_title(void);

double firefox_get_position(void);

#endif
#endif /* _FIREFOX_EXTENSION_BRIDGE_INCLUDED */
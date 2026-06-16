
#ifdef _FIREFOX_EXTENSION_BRIDGE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "../common_utils/string_utils.c"
#include "thing_for_firefox_extension_bridge_thingy.h"

static firefox_metadata firefox_data = {0};
static volatile sig_atomic_t bridge_running = 1;

void firefox_bridge_cleanup(int sig) {
    (void)sig;
    bridge_running = 0;
}

/* Simple JSON string value parser */
static char* firefox_extract_json_value(const char *json, const char *key) {
    if (!json || !key) return NULL;

    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":\"", key);

    const char *start = strstr(json, search_pattern);
    if (!start) {
        #ifdef _DEBUG
        fprintf(stderr, "DEBUG: Key '%s' not found in JSON\n", key);
        #endif
        return NULL;
    }

    start += strlen(search_pattern);
    const char *end = start;

    /* Find the closing quote, handling escapes */
    while (*end && *end != '"') {
        if (*end == '\\' && *(end + 1)) {
            end += 2;
        } else {
            end++;
        }
    }

    size_t len = end - start;

    #ifdef _DEBUG
    fprintf(stderr, "DEBUG: Extracted key '%s': len=%zu, value='%.*s'\n", key, len, (int)len, start);
    #endif

    char *result = malloc(len + 1);
    if (!result) return NULL;

    if (len > 0) {
        memcpy(result, start, len);
    }
    result[len] = '\0';
    return result;
}

/* Free old metadata and store new values */
static void firefox_store_metadata(const char *title, const char *artist,
                                   const char *album, const char *url) {
    if (firefox_data.title) free(firefox_data.title);
    if (firefox_data.artist) free(firefox_data.artist);
    if (firefox_data.album) free(firefox_data.album);
    if (firefox_data.url) free(firefox_data.url);

    firefox_data.title = title ? strdup(title) : NULL;
    firefox_data.artist = artist ? strdup(artist) : NULL;
    firefox_data.album = album ? strdup(album) : NULL;
    firefox_data.url = url ? strdup(url) : NULL;
}

/* Send HTTP response */
static void send_http_response(int client, const char *body, const char *content_type) {
    size_t body_len = strlen(body);
    char header[512];
    int n = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "\r\n",
        content_type, body_len);

    #ifdef _DEBUG
    fprintf(stderr, "DEBUG: send_http_response - headers: %d bytes, body: %zu bytes\n", n, body_len);
    fprintf(stderr, "DEBUG: send_http_response - body content: %s\n", body);
    #endif

    ssize_t written = write(client, header, n);
    if (written != n) {
        #ifdef _DEBUG
        fprintf(stderr, "DEBUG: Warning - only wrote %zd of %d header bytes\n", written, n);
        #endif
    }
    written = write(client, body, body_len);
    if (written != (ssize_t)body_len) {
        #ifdef _DEBUG
        fprintf(stderr, "DEBUG: Warning - only wrote %zd of %zu body bytes\n", written, body_len);
        #endif
    }
}

/* Send OPTIONS response for CORS preflight */
static void send_cors_preflight(int client) {
    const char *response =
        "HTTP/1.1 204 No Content\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "\r\n";
    write(client, response, strlen(response));
}

/* Get metadata as JSON response */
static void firefox_get_metadata_json(char *buffer, size_t size) {
    snprintf(buffer, size,
        "{\"title\":\"%s\",\"artist\":\"%s\",\"album\":\"%s\",\"url\":\"%s\"}",
        firefox_data.title ? firefox_data.title : "",
        firefox_data.artist ? firefox_data.artist : "",
        firefox_data.album ? firefox_data.album : "",
        firefox_data.url ? firefox_data.url : "");
}

/* Handle incoming HTTP requests */
static void firefox_handle_request(int client, const char *request) {
    /* Parse request line */
    char method[16] = {0};
    char path[256] = {0};
    sscanf(request, "%15s %255s", method, path);

    #ifdef _DEBUG
    fprintf(stderr, "DEBUG: Received %s request for %s\n", method, path);
    fprintf(stderr, "DEBUG: Current metadata - artist: '%s', title: '%s'\n",
        firefox_data.artist ? firefox_data.artist : "",
        firefox_data.title ? firefox_data.title : "");
    #endif

    /* Handle CORS preflight */
    if (strcmp(method, "OPTIONS") == 0) {
        send_cors_preflight(client);
        return;
    }

    if (strcmp(method, "POST") == 0 && strcmp(path, "/media") == 0) {
        /* Find the body (after double CRLF) */
        const char *body_start = strstr(request, "\r\n\r\n");
        if (body_start) {
            body_start += 4;

            #ifdef _DEBUG
            fprintf(stderr, "DEBUG: POST body: %s\n", body_start);
            #endif

            /* Parse JSON from POST body */
            char *title = firefox_extract_json_value(body_start, "title");
            char *artist = firefox_extract_json_value(body_start, "artist");
            char *album = firefox_extract_json_value(body_start, "album");
            char *url = firefox_extract_json_value(body_start, "url");

            #ifdef _DEBUG
            fprintf(stderr, "DEBUG: Parsed - title: '%s', artist: '%s'\n",
                title ? title : "", artist ? artist : "");
            #endif

            /* Store and respond */
            firefox_store_metadata(title, artist, album, url);

            char response_body[BUFFER_SIZE];
            firefox_get_metadata_json(response_body, sizeof(response_body));
            send_http_response(client, response_body, "application/json");

            if (title) free(title);
            if (artist) free(artist);
            if (album) free(album);
            if (url) free(url);
        }
    } else if (strcmp(method, "GET") == 0 && strcmp(path, "/media") == 0) {
        /* Return current metadata */
        char response_body[BUFFER_SIZE];
        firefox_get_metadata_json(response_body, sizeof(response_body));
        #ifdef _DEBUG
        fprintf(stderr, "DEBUG: Sending GET response: %s\n", response_body);
        #endif
        send_http_response(client, response_body, "application/json");
    } else {
        /* 404 response */
        const char *not_found = "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 9\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n"
            "\r\nNot Found";
        write(client, not_found, strlen(not_found));
    }
}

/* Main Firefox bridge server - runs indefinitely */
int firefox_bridge_server(void) {
    struct sigaction sa = {0};
    sa.sa_handler = firefox_bridge_cleanup;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(FIREFOX_BRIDGE_PORT);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Firefox bridge server listening on port %d\n", FIREFOX_BRIDGE_PORT);
    fflush(stderr);

    while (bridge_running) {
        int client = accept(server_fd, NULL, NULL);
        if (client < 0) continue;

        #ifdef _DEBUG
        fprintf(stderr, "DEBUG: New client connection\n");
        fflush(stderr);
        #endif

        char request[BUFFER_SIZE] = {0};
        ssize_t n = read(client, request, BUFFER_SIZE - 1);
        if (n > 0) {
            request[n] = '\0';
            firefox_handle_request(client, request);
        } else {
            #ifdef _DEBUG
            fprintf(stderr, "DEBUG: No data read from client\n");
            #endif
        }

        close(client);
    }

    close(server_fd);

    /* Cleanup */
    if (firefox_data.title) free(firefox_data.title);
    if (firefox_data.artist) free(firefox_data.artist);
    if (firefox_data.album) free(firefox_data.album);
    if (firefox_data.url) free(firefox_data.url);

    return EXIT_SUCCESS;
}

/* Getter functions for utils.c to call - connect to server and fetch data */
static char* firefox_http_get_metadata(void) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        #ifdef _DEBUG
        fprintf(stderr, "DEBUG: Firefox bridge socket creation failed\n");
        #endif
        return NULL;
    }

    /* Set receive timeout */
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(FIREFOX_BRIDGE_PORT);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        #ifdef _DEBUG
        fprintf(stderr, "DEBUG: Firefox bridge connection failed\n");
        #endif
        close(sock);
        return NULL;
    }

    /* Send GET request */
    const char *request = "GET /media HTTP/1.1\r\nHost: 127.0.0.1:3847\r\n"
                         "Connection: close\r\n\r\n";
    if (write(sock, request, strlen(request)) < 0) {
        #ifdef _DEBUG
        fprintf(stderr, "DEBUG: Firefox bridge write failed\n");
        #endif
        close(sock);
        return NULL;
    }

    /* Read response in a loop until we get all data */
    char *response = malloc(BUFFER_SIZE);
    if (!response) {
        close(sock);
        return NULL;
    }

    ssize_t total_read = 0;
    ssize_t n;
    while (total_read < BUFFER_SIZE - 1) {
        n = read(sock, response + total_read, BUFFER_SIZE - 1 - total_read);
        if (n <= 0) {
            break;
        }
        total_read += n;
    }
    close(sock);

    if (total_read <= 0) {
        #ifdef _DEBUG
        fprintf(stderr, "DEBUG: Firefox bridge read failed or no data\n");
        #endif
        free(response);
        return NULL;
    }

    response[total_read] = '\0';

    #ifdef _DEBUG
    fprintf(stderr, "DEBUG: Firefox bridge raw response (%zd bytes total):\n", total_read);
    fprintf(stderr, "DEBUG: HEX DUMP: ");
    for (ssize_t i = 0; i < total_read && i < 300; i++) {
        fprintf(stderr, "%02x ", (unsigned char)response[i]);
        if ((i + 1) % 32 == 0) fprintf(stderr, "\n              ");
    }
    fprintf(stderr, "\n");
    #endif

    /* Find the body (after double CRLF) */
    char *body_start = strstr(response, "\r\n\r\n");
    if (body_start) {
        #ifdef _DEBUG
        fprintf(stderr, "DEBUG: Found CRLF separator at offset %ld\n", body_start - response);
        fprintf(stderr, "DEBUG: Bytes after CRLF separator: %zd\n", total_read - (body_start - response) - 4);
        #endif

        body_start += 4;

        #ifdef _DEBUG
        fprintf(stderr, "DEBUG: Extracted body (%zu chars): %s\n", strlen(body_start), body_start);
        #endif

        char *body = malloc(strlen(body_start) + 1);
        strcpy(body, body_start);
        free(response);
        return body;
    }

    #ifdef _DEBUG
    fprintf(stderr, "DEBUG: No \\r\\n\\r\\n found in response\n");
    #endif
    free(response);
    return NULL;
}

str firefox_get_artist(void) {
    char *json = firefox_http_get_metadata();
    if (!json) {
        #ifdef _DEBUG
        fprintf(stderr, "DEBUG: firefox_get_artist - no JSON response\n");
        #endif
        return str_create("");
    }

    char *artist = firefox_extract_json_value(json, "artist");
    #ifdef _DEBUG
    fprintf(stderr, "DEBUG: firefox_get_artist extracted: '%s'\n", artist ? artist : "(null)");
    #endif

    free(json);

    str result = str_create(artist ? artist : "");
    if (artist) free(artist);
    return result;
}

str firefox_get_title(void) {
    char *json = firefox_http_get_metadata();
    if (!json) {
        #ifdef _DEBUG
        fprintf(stderr, "DEBUG: firefox_get_title - no JSON response\n");
        #endif
        return str_create("");
    }

    char *title = firefox_extract_json_value(json, "title");
    #ifdef _DEBUG
    fprintf(stderr, "DEBUG: firefox_get_title extracted: '%s'\n", title ? title : "(null)");
    #endif

    free(json);

    str result = str_create(title ? title : "");
    if (title) free(title);
    return result;
}

double firefox_get_position(void) {
    /* Firefox doesn't provide position data through extension */
    return 0.0;
}

#endif /* _FIREFOX_EXTENSION_BRIDGE */


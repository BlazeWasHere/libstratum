//          Copyright Blaze 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libstratum/stratum.h"

#include "libstratum/connection.h"
#include "libstratum/jsmn.h"

#ifdef ENABLE_DEBUG_LOGGING
#define DEBUG_LOG(...)                                                         \
    printf(__VA_ARGS__);                                                       \
    puts("");
#else
#define DEBUG_LOG(...)
#endif

#ifdef ENABLE_CRITICAL_LOGGING
#define CRITICAL_LOG(...) warn(__VA_ARGS__)
#else
#define CRITICAL_LOG(...)
#endif

#define BUFSIZE 1024

char *stratum_serialize_data(stratum_data_t *data) {
    size_t size =
        snprintf(NULL, 0, "{\"id\": %d, \"method\": \"%s\", \"params\": %s}\n",
                 data->id, data->method, data->params);

    char *dumped = calloc(1, size + 1);

    snprintf(dumped, size + 1,
             "{\"id\": %d, \"method\": \"%s\", \"params\": %s}\n", data->id,
             data->method, data->params);

    return dumped;
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0)
        return 0;

    return -1;
}

stratum_response_t *stratum_parse_response(const char *data) {
    // Assume data has been split by '\n'.
    assert(strchr(data, '\n') == NULL);

    stratum_response_t *stratum_data = calloc(1, sizeof(stratum_response_t));
    jsmn_parser parser;
    jsmntok_t t[128];

    jsmn_init(&parser);

    int ret =
        jsmn_parse(&parser, data, strlen(data), t, sizeof(t) / sizeof(t[0]));

    if (ret < 1 || t[0].type != JSMN_OBJECT) {
        // failed to parse object
        stratum_data->id = -1;
        return stratum_data;
    }

    for (int i = 1; i < ret; i++) {
        jsmntok_t *x = &t[i + 1];
        size_t size;
        char *tmp = NULL;

        // printf("key: %.*s ", t[i].end - t[i].start, data + t[i].start);
        // printf("value: %.*s\n", x->end - x->start, data + x->start);

        if (jsoneq(data, &t[i], "id") == 0) {
            size =
                snprintf(NULL, 0, "%.*s", x->end - x->start, data + x->start);
            tmp = calloc(1, size + 1);
            snprintf(tmp, size + 1, "%.*s", x->end - x->start, data + x->start);

            if (strncmp(data + x->start, "null", x->end - x->start) == 0)
                stratum_data->id = 0;
            else
                stratum_data->id = strtol(tmp, NULL, 10);

        } else if (jsoneq(data, &t[i], "result") == 0) {
            if (x->type != JSMN_ARRAY) {
                size = snprintf(NULL, 0, "%.*s", x->end - x->start,
                                data + x->start);
                tmp = calloc(1, size + 1);
                snprintf(tmp, size + 1, "%.*s", x->end - x->start,
                         data + x->start);

                stratum_data->result[0] = strdup(tmp);
                stratum_data->result[1] = NULL;
            } else {
                for (int j = 0; j < x->size; j++) {
                    jsmntok_t *g = &t[i + j + 2];
                    size = snprintf(NULL, 0, "%.*s", g->end - g->start,
                                    data + g->start);
                    stratum_data->result[j] = calloc(1, size + 1);

                    snprintf(stratum_data->result[j], size + 1, "%.*s",
                             g->end - g->start, data + g->start);
                }

                i += t[i + 1].size + 1;
            }
        } else if (jsoneq(data, &t[i], "method") == 0) {
            size =
                snprintf(NULL, 0, "%.*s", x->end - x->start, data + x->start);
            tmp = calloc(1, size + 1);
            snprintf(tmp, size + 1, "%.*s", x->end - x->start, data + x->start);

            stratum_data->method = strdup(tmp);
        } else if (jsoneq(data, &t[i], "error") == 0) {
            if (x->type != JSMN_ARRAY)
                continue; // We expected an array of strings.

            for (int j = 0; j < x->size; j++) {
                jsmntok_t *g = &t[i + j + 2];
                size = snprintf(NULL, 0, "%.*s", g->end - g->start,
                                data + g->start);
                stratum_data->error[j] = calloc(1, size + 1);

                snprintf(stratum_data->error[j], size + 1, "%.*s",
                         g->end - g->start, data + g->start);
            }

            i += t[i + 1].size + 1;
        } else if (jsoneq(data, &t[i], "params") == 0) {
            if (x->type != JSMN_ARRAY)
                continue; // We expected an array of strings.

            for (int j = 0; j < x->size; j++) {
                jsmntok_t *g = &t[i + j + 2];
                size = snprintf(NULL, 0, "%.*s", g->end - g->start,
                                data + g->start);
                stratum_data->params[j] = calloc(1, size + 1);

                snprintf(stratum_data->params[j], size + 1, "%.*s",
                         g->end - g->start, data + g->start);
            }

            i += t[i + 1].size + 1;
        } else {
            if (x->type == JSMN_STRING) {
                DEBUG_LOG("Did not match any in parsing switch, got `%.*s`",
                          x->end - x->start, data + x->start);
            }
        }

        if (tmp != NULL) {
            free(tmp);
            i++;
        }
    }

    return stratum_data;
}

void stratum_mining_subscribe(int socket, const char *user_agent,
                              const char *session_id, const char *host,
                              const char *port, stratum_cb_t cb) {
    size_t size = snprintf(NULL, 0, "[\"%s\", \"%s\", \"%s\", %s]", user_agent,
                           session_id, host, port) +
                  1;
    char *params = calloc(1, size);
    snprintf(params, size, "[\"%s\", \"%s\", \"%s\", %s]", user_agent,
             session_id, host, port);

    stratum_data_t data = {
        .id = 1,
        .method = "mining.subscribe",
        .params = params,
    };

    stratum_send_and_handle_data(socket, &data, cb);
}

void stratum_mining_authorize(int socket, const char *username,
                              const char *password, stratum_cb_t cb) {
    size_t size = snprintf(NULL, 0, "[\"%s\", \"%s\"]", username, password);
    char *params = calloc(1, size + 1);
    snprintf(params, size + 1, "[\"%s\", \"%s\"]", username, password);

    stratum_data_t data = {
        .id = 2,
        .method = "mining.authorize",
        .params = params,
    };

    stratum_send_and_handle_data(socket, &data, cb);
}

void stratum_mining_submit(int socket, const char *worker, const char *job_id,
                           const char *time, const char *nonce_2,
                           char *solution, stratum_cb_t cb) {
    size_t size = snprintf(NULL, 0, "[\"%s\", \"%s\", \"%s\", \"%s\", \"%s\"]",
                           worker, job_id, time, nonce_2, solution);
    char *params = calloc(1, size + 1);
    snprintf(params, size + 1, "[\"%s\", \"%s\", \"%s\", \"%s\", \"%s\"]",
             worker, job_id, time, nonce_2, solution);

    stratum_data_t data = {
        .id = 2,
        .method = "mining.submit",
        .params = params,
    };

    stratum_send_and_handle_data(socket, &data, cb);
}

void stratum_send_and_handle_data(int socket, stratum_data_t *data,
                                  stratum_cb_t cb) {
    char *str = stratum_serialize_data(data);
    char *buf = calloc(1, BUFSIZE);

    socket_send(socket, str);
    socket_read(socket, buf, BUFSIZE);

    char *_str, *token;

    _str = strdup(buf);
    // Split by '\n'.
    while ((token = strsep(&_str, "\n"))) {
        if (!strlen(token))
            continue;

        DEBUG_LOG("Parsing %s", token);
        stratum_response_t *res = stratum_parse_response(token);
        DEBUG_LOG(
            "Received: id %ld, method: %s, result: [%s, %s], errors: [%s, "
            "%s, %s]",
            res->id, res->method, res->result[0], res->result[1], res->error[0],
            res->error[1], res->error[2]);

        if (res->id == -1) {
            // TODO(blaze): resend data?
            err(EXIT_FAILURE,
                "Failed to parse the response from the server.\n"
                "Sent to the server: %s",
                str);
        } else if (res->id == 0) {
            // Params have been given, print them to DEBUG.
            DEBUG_LOG("params: [%s, %s, %s, %s, %s, %s, %s, %s]",
                      res->params[0], res->params[1], res->params[2],
                      res->params[3], res->params[4], res->params[5],
                      res->params[6], res->params[7]);
        }

        if (cb != NULL)
            cb(res, socket);

        for (int i = 0; i < 2; i++)
            if (res->result[i] != NULL)
                free(res->result[i]);

        for (int i = 0; i < 8; i++)
            free(res->params[i]);

        free(res->error[0]);
        free(res->error[1]);
        free(res->error[2]);
        free(res->method);
    }

    free(_str);
    free(buf);
}

const char *stratum_error_code_to_string(uint8_t code) {
    // https://zips.z.cash/zip-0301#error-objects
    assert(code >= 20 && code <= 25);

    switch (code) {
    case 20:
        return "Other/Unknown";
    case 21:
        return "Stale Job";
    case 22:
        return "Duplicate Share";
    case 23:
        return "Low Difficulty Share";
    case 24:
        return "Unauthorized Worker";
    case 25:
        return "Not Subscribed";
    default:
        DEBUG_LOG("Got code: %d which was not handled by `%s()`", code,
                  __func__);
        return "UNKNOWN";
    }
}

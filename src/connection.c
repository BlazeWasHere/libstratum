//          Copyright Blaze 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <err.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "libstratum/connection.h"

#ifdef ENABLE_DEBUG_LOGGING
#define DEBUG_LOG(...)                                                         \
    {                                                                          \
        printf(__VA_ARGS__);                                                   \
        puts("");                                                              \
    }
#else
#define DEBUG_LOG(...)
#endif

#ifdef ENABLE_CRITICAL_LOGGING
#define CRITICAL_LOG(...) warn(__VA_ARGS__)
#else
#define CRITICAL_LOG(...)
#endif

#define RETRY_COUNT 3

int socket_init(const char *hostname, const char *port) {
    struct addrinfo hints, *res, *p;
    int ret, sock = -1;
    void *ptr;
    // overhead for ipv6 addresses.
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((ret = getaddrinfo(hostname, port, &hints, &res)) != 0)
        err(EXIT_FAILURE, "Failed to convert hostname to ip: %s",
            gai_strerror(ret));

    p = res;

    while (p) {
        inet_ntop(p->ai_family, p->ai_addr->sa_data, ipstr, sizeof(ipstr));

        switch (p->ai_family) {
        case AF_INET:
            // ipv4
            ptr = &((struct sockaddr_in *)p->ai_addr)->sin_addr;
            break;
        case AF_INET6:
            // ipv6
            ptr = &((struct sockaddr_in6 *)p->ai_addr)->sin6_addr;
            break;
        default:
            err(EXIT_FAILURE, "sanity check? got %d as `p->ai_family`",
                p->ai_family);
        }

        inet_ntop(p->ai_family, ptr, ipstr, sizeof(ipstr));
        DEBUG_LOG("IPv%d address: %s (%s)", p->ai_family == PF_INET6 ? 6 : 4,
                  ipstr, p->ai_canonname);

        if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
            -1) {
            CRITICAL_LOG("Failed to initialize the socket, retrying...");
        } else if (connect(sock, p->ai_addr, p->ai_addrlen) == -1) {
            CRITICAL_LOG("Failed to connect, retrying...");
        } else {
            DEBUG_LOG("Connected to the server - %s / %s:%s", hostname, ipstr,
                      port);
            break;
        }

        p = p->ai_next;
    }

    freeaddrinfo(res);

    return sock;
}

void socket_send(int socket, const char *data) {
    int retries = 0;
    ssize_t ret = -1;

    do {
        if (retries == RETRY_COUNT) {
            err(EXIT_FAILURE, "Aborting after %d retries.", RETRY_COUNT);
        } else if (retries > 0) {
            DEBUG_LOG("Retry count: %d, resending data: %s", retries, data);
            perror("socket_send");
        }

        ret = send(socket, data, strlen(data), 0);
        DEBUG_LOG("Sending data to the socket fd(%d)", socket);

        if (ret == -1)
            retries++;

    } while (ret == -1);
}

void socket_read(int socket, void *buffer, size_t bufsize) {
    ssize_t ret = read(socket, buffer, bufsize);

    if (ret == -1)
        err(EXIT_FAILURE, "Read failure for fd(%d) with bufsize (%ld)", socket,
            bufsize);
    else if (ret < (ssize_t)bufsize) {
        DEBUG_LOG("Read %ld bytes with a %ld buffer size", ret, bufsize);
    }
}

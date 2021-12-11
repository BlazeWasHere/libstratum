//          Copyright Blaze 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef LIBSTRATUM_CONNECTION_H
#define LIBSTRATUM_CONNECTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <arpa/inet.h>

/* create a socket, and return the fd */
int socket_init(const char *hostname, const char *port);

/* send `data` to the socket */
void socket_send(int socket, const char *data);

/* write `bufsize` amount of data received by the socket into `buffer` */
void socket_read(int socket, void *buffer, size_t bufsize);

#ifdef __cplusplus
}
#endif

#endif /* LIBSTRATUM_CONNECTION_H */

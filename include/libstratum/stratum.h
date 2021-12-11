//          Copyright Blaze 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef LIBSTRATUM_STRATUM_H
#define LIBSTRATUM_STRATUM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define LIBSTRATUM_VERSION_MAJOR 0
#define LIBSTRATUM_VERSION_MINOR 0
#define LIBSTRATUM_VERSION_PATCH 1
#define LIBSTRATUM_VERSION_NUMBER                                              \
    (LIBSTRATUM_VERSION_MAJOR * 100 * 100 + LIBSTRATUM_VERSION_MINOR * 100 +   \
     LIBSTRATUM_VERSION_PATCH)

typedef struct {
    // 1 to 4 + null are the only valid ids, 0 = null
    uint8_t id;
    const char *method;
    // "[\"...\", \"...\"]"
    char *params;
} stratum_data_t;

typedef struct {
    // -1 = parsing error from client (us)
    long id;
    char *method;
    char *result[2];
    char *error[3];
    // server should never send more than 8 params. mining.notify()
    char *params[8];
} stratum_response_t;

typedef void (*stratum_cb_t)(stratum_response_t *evt, int socket);

/* serialize the data, so that it is ready to be sent to the socket */
char *stratum_serialize_data(stratum_data_t *data);

/* parses the JSON data into an alloc'd stratum_data_t */
stratum_response_t *stratum_parse_response(const char *data);

/**
 * https://zips.z.cash/zip-0301#mining-subscribe
 *
 * SESSION_ID (str)
 *    The id for a previous session that the miner wants to resume
 *    (e.g. after a temporary network disconnection)
 *    (see https://zips.z.cash/zip-0301#session-resuming).
 *    This MAY be null indicating that the miner wants to start a new session.
 * CONNECT_HOST (str)
 *   The host that the miner is connecting to (from the server URL).
 *   Example: pool.example.com
 * CONNECT_PORT (int)
 *   The port that the miner is connecting to (from the server URL).
 *   Example: 3337
 * MINER_USER_AGENT (str)
 *   A free-form string specifying the type and version of the mining software.
 *	 Recommended syntax is the User Agent format used by Zcash nodes.
 *   Example: MagicBean/1.0.0
 **/
void stratum_mining_subscribe(int socket, const char *user_agent,
                              const char *session_id, const char *host,
                              const char *port, stratum_cb_t cb);

/**
 * https://zips.z.cash/zip-0301#mining-authorize
 *
 * WORKER_NAME (str)
 *   The worker name.
 * WORKER_PASSWORD (str)
 *     The worker password.
 **/

void stratum_mining_authorize(int socket, const char *username,
                              const char *password, stratum_cb_t cb);

void stratum_send_and_handle_data(int socket, stratum_data_t *data,
                                  stratum_cb_t cb);

/* convert a stratum server error code to a human readable string */
const char *stratum_error_code_to_string(uint8_t code);

/**
 * https://zips.z.cash/zip-0301#mining-submit
 *
 * WORKER_NAME (str)
 *   A previously-authenticated worker name.
 *   Servers MUST NOT accept submissions from unauthenticated workers.
 * JOB_ID (str)
 *   The id of the job this submission is for.
 *   Miners MAY make multiple submissions for a single job id.
 * TIME (hex)
 *   The block time used in the submission, encoded as in a block header.
 *   MAY be enforced by the server to be unchanged.
 * NONCE_2 (hex)
 *   The second part of the block header nonce (see Nonce Parts).
 * EQUIHASH_SOLUTION (hex)
 *   The Equihash solution, encoded as in a block header
 *   (including the compactSize at the beginning in canonical form
 *   https://en.bitcoin.it/wiki/Protocol_documentation#Variable_length_integer)
 **/
void stratum_mining_submit(int socket, const char *worker, const char *job_id,
                           const char *time, const char *nonce_2,
                           char *solution, stratum_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif /* LIBSTRATUM_STRATUM_H */

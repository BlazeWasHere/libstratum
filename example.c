// Example usage of a client logging into a zcash.flypool.org pool and
// submitting.

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libstratum/connection.h"
#include "libstratum/stratum.h"

const char *username = "t1QbTtc3ZtjovbpSNgwcvSczWMEMKxE2AuE";
const char *hostname = "us1-zcash.flypool.org";
const char *port = "3333";

// Very minimal callback.
static void cb(stratum_response_t *res, int socket) {
    (void)socket;

    switch (res->id) {
    case 0:
        printf("server gave us a notification: method (%s)\n", res->method);
        // Ideally params will be handled and edit internal miner variables
        // if needed, such as a new target on method `mining.set_target`.
        break;
    case 1:
        printf("server set out session_id as `%s` and our nonce_1 as `%s`\n",
               res->result[0], res->result[1]);
        break;
    case 2:
        // This MUST be true if authorization succeeded.
        // It MUST be null if there was an error.
        if (strcmp(res->result[0], "null") == 0) {
            // TODO: casting to lower precision is bad.
            uint8_t code = strtol(res->error[0], NULL, 0);

            // 20: Other/Unknown error
            if (code != 20)
                warnx("Server replied with %s",
                      stratum_error_code_to_string(code));
            else
                warnx("Server replied with a custom error (%s)",
                      res->result[1]);

            warnx("Error message: \"%s\", Server traceback: \"%s\"",
                  res->error[1], res->error[2]);
        } else if (strcmp(res->result[0], "true") == 0) {
            printf("Successfully authorized as a worker\n");
        } else {
            errx(EXIT_FAILURE, "Received `%s` which we did not handle",
                 res->result[0]);
        }
        break;
    default:
        warnx("Got id: %ld which was not handled by `%s()`", res->id, __func__);
        break;
    }
}

int main(void) {
    int socket = socket_init(hostname, port);
    stratum_mining_subscribe(socket, "dummy useragent", "null", hostname, port,
                             cb);
    stratum_mining_authorize(socket, username, "", cb);
    // A mock submission - obviously invalid.
    printf("Submitting a mock submission\n");
    stratum_mining_submit(socket, username, "69", "420", "101", "foo", cb);
    // Ditto.
    stratum_mining_submit(socket, username, "a", "b", "c", "d", cb);
}

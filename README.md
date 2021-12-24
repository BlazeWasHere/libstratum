# libstratum

A minimal [stratum protocol](https://en.bitcoin.it/wiki/Stratum_mining_protocol) wrapper tailored for the zcash variant.

# API

```c
void stratum_mining_subscribe(int socket, const char *user_agent,
                              const char *session_id, const char *host,
                              const char *port, stratum_cb_t cb);

void stratum_mining_authorize(int socket, const char *username,
                              const char *password, stratum_cb_t cb);

void stratum_send_and_handle_data(int socket, stratum_data_t *data,
                                  stratum_cb_t cb);

void stratum_mining_submit(int socket, const char *worker, const char *job_id,
                           const char *time, const char *nonce_2,
                           char *solution, stratum_cb_t cb);
```

View all exported functions [here](https://github.com/blazewashere/libstratum/tree/master/include/libstratum)

# Usage

View [example.c](https://github.com/blazewashere/libstratum/blob/master/example.c)

# Example

Example usage of a client logging into a zcash.flypool.org pool and
submitting an invalid response.

```sh
$ make example
cc -Iinclude -c src/connection.c -o object/connection.o -D_FORTIFY_SOURCE=2 -fstack-clash-protection -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wstrict-overflow=5
[...]
$ ./example
server set out session_id as `2401383833` and our nonce_1 as `2401383833`
Successfully authorized as a worker
server gave us a notification: method (mining.set_target)
Submitting a mock submission
server gave us a notification: method (mining.notify)
example: Server replied with a custom error ((null))
example: Error message: "Invalid solution!", Server traceback: "null
```

# Note

You can enable debugging by compiling with the `DEBUG=1` flag.
This enables the `-g` flag on gcc and internal logging methods in libstratum.

```sh
$ make DEBUG=1
```

# Thanks

[jsmn.h](https://github.com/zserge/jsmn)
[zip-301](https://zips.z.cash/zip-0301)

# License

[BSL-1.0 License](https://github.com/BlazeWasHere/libstratum/blob/master/LICENSE)

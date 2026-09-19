#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <stddef.h>


typedef struct tcp_server tcp_server_t;

typedef enum {
    TCP_CLIENT_CONNECTED = 0,
    TCP_CLIENT_DATA,
    TCP_CLIENT_DISCONNECTED
} tcp_client_event_t;


/**
 * @brief Handle TCP client events.
 *
 * The TCP server reports client connection,
 * received data, and disconnection events
 * through this callback.
 *
 * @param client_fd Client socket file descriptor.
 * @param event     Client event type.
 * @param data      Received data buffer.
 *                  Valid when event is TCP_CLIENT_DATA.
 * @param length    Length of received data.
 * @param arg       User-defined callback argument.
 *
 * @return 0 on success, -1 on failure.
 */
typedef int (*tcp_client_handler_t)(int client_fd, tcp_client_event_t event, const char *data, size_t length, void *arg);


/**
 * @brief Create a TCP server.
 *
 * @param ip   Server bind IP address.
 * @param port Server listening port.
 *
 * @return TCP server pointer on success,
 *         NULL on failure.
 */
tcp_server_t *tcp_server_create(const char *ip, int port);


/**
 * @brief Destroy a TCP server.
 *
 * The server should be stopped before destruction.
 *
 * @param server TCP server pointer.
 */
void tcp_server_destroy(tcp_server_t *server);


/**
 * @brief Set client event handler.
 *
 * @param server  TCP server pointer.
 * @param handler Client event callback.
 * @param arg     User-defined callback argument.
 *
 * @return 0 on success, -1 on failure.
 */
int tcp_server_set_handler(tcp_server_t *server, tcp_client_handler_t handler, void *arg);


/**
 * @brief Start TCP server.
 *
 * This function starts listening for client
 * connections and normally blocks until the
 * server is stopped.
 *
 * @param server TCP server pointer.
 *
 * @return 0 on normal stop, -1 on failure.
 */
int tcp_server_start(tcp_server_t *server);


/**
 * @brief Stop TCP server.
 *
 * Stop the listening socket and terminate
 * the server accept loop.
 *
 * @param server TCP server pointer.
 */
void tcp_server_stop(tcp_server_t *server);


/**
 * @brief Send data to a TCP client.
 *
 * This function hides the underlying socket
 * send operation from upper modules.
 *
 * @param client_fd Client socket file descriptor.
 * @param data      Data to send.
 * @param length    Number of bytes to send.
 *
 * @return 0 on success, -1 on failure.
 */
int tcp_server_send(int client_fd, const char *data, size_t length);


#endif
#include "tcp_server.h"

#include "utils/log/log.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define TCP_SERVER_BUFFER_SIZE 128
#define TCP_SERVER_BACKLOG     5


/* ==============================
 * Internal Structure
 * ============================== */

struct tcp_server {
    int server_fd;

    char ip[INET_ADDRSTRLEN];
    int port;

    int running;

    tcp_client_handler_t handler;
    void *handler_arg;
};


/* ==============================
 * Client Context
 * ============================== */

typedef struct {
    tcp_server_t *server;
    int client_fd;
} tcp_client_context_t;


/* ==============================
 * Internal Functions
 * ============================== */

static void *tcp_client_thread(void *arg)
{
    tcp_client_context_t *context =
        (tcp_client_context_t *)arg;

    tcp_server_t *server = context->server;
    int client_fd = context->client_fd;

char buffer[TCP_SERVER_BUFFER_SIZE];

free(context);

log_info("Client connected: fd=%d", client_fd);

/* Notify upper layer that a client has connected. */
if (server->handler != NULL) {
    int ret = server->handler(
        client_fd,
        TCP_CLIENT_CONNECTED,
        NULL,
        0,
        server->handler_arg
    );

    if (ret < 0) {
        log_warn(
            "Client connected handler returned error: fd=%d",
            client_fd
        );
    }
}

while (server->running) {

        ssize_t received = recv(
            client_fd,
            buffer,
            sizeof(buffer) - 1,
            0
        );

        if (received > 0) {

            buffer[received] = '\0';

            log_info(
                "Received %zd bytes from client fd=%d",
                received,
                client_fd
            );

        if (server->handler != NULL) {

            int ret = server->handler(
                client_fd,
                TCP_CLIENT_DATA,
                buffer,
                (size_t)received,
                server->handler_arg
            );

            if (ret < 0) {
                log_warn(
                    "Client data handler returned error: fd=%d",
                    client_fd
                );
            }
        }

        } else if (received == 0) {

            log_info(
                "Client disconnected: fd=%d",
                client_fd
            );

            if (server->handler != NULL) {
                int ret = server->handler(
                    client_fd,
                    TCP_CLIENT_DISCONNECTED,
                    NULL,
                    0,
                    server->handler_arg
                );

                if (ret < 0) {
                    log_warn(
                        "Client disconnected handler returned error: fd=%d",
                        client_fd
                    );
                }
            }

            break;

        } else {

            if (errno == EINTR) {
                continue;
            }

            log_error(
                "recv failed: fd=%d, error=%s",
                client_fd,
                strerror(errno)
            );

            break;
        }
    }

    close(client_fd);

    log_info("Client thread exited: fd=%d", client_fd);

    return NULL;
}


/* ==============================
 * Public Functions
 * ============================== */

tcp_server_t *tcp_server_create(
    const char *ip,
    int port
)
{
    if (ip == NULL || port <= 0 || port > 65535) {
        log_error("Invalid TCP server parameters");
        return NULL;
    }

    tcp_server_t *server =
        calloc(1, sizeof(tcp_server_t));

    if (server == NULL) {
        log_error("Failed to allocate tcp_server");
        return NULL;
    }

    strncpy(
        server->ip,
        ip,
        sizeof(server->ip) - 1
    );

    server->ip[sizeof(server->ip) - 1] = '\0';

    server->port = port;
    server->server_fd = -1;
    server->running = 0;
    server->handler = NULL;
    server->handler_arg = NULL;

    return server;
}


int tcp_server_set_handler(
    tcp_server_t *server,
    tcp_client_handler_t handler,
    void *arg
)
{
    if (server == NULL) {
        return -1;
    }

    server->handler = handler;
    server->handler_arg = arg;

    return 0;
}


int tcp_server_start(tcp_server_t *server)
{
    if (server == NULL) {
        return -1;
    }

    if (server->running) {
        log_warn("TCP server is already running");
        return -1;
    }


    /* ==============================
     * Create Socket
     * ============================== */

    int server_fd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (server_fd < 0) {

        log_error(
            "socket() failed: %s",
            strerror(errno)
        );

        return -1;
    }


    /* ==============================
     * SO_REUSEADDR
     * ============================== */

    int reuse = 1;

    if (setsockopt(
            server_fd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuse,
            sizeof(reuse)) < 0) {

        log_error(
            "setsockopt(SO_REUSEADDR) failed: %s",
            strerror(errno)
        );

        close(server_fd);

        return -1;
    }


    /* ==============================
     * Bind
     * ============================== */

    struct sockaddr_in server_addr;

    memset(
        &server_addr,
        0,
        sizeof(server_addr)
    );

    server_addr.sin_family = AF_INET;
    server_addr.sin_port =
        htons((uint16_t)server->port);


    if (inet_pton(
            AF_INET,
            server->ip,
            &server_addr.sin_addr) <= 0) {

        log_error(
            "Invalid server IP address: %s",
            server->ip
        );

        close(server_fd);

        return -1;
    }


    if (bind(
            server_fd,
            (struct sockaddr *)&server_addr,
            sizeof(server_addr)) < 0) {

        log_error(
            "bind() failed: %s",
            strerror(errno)
        );

        close(server_fd);

        return -1;
    }


    /* ==============================
     * Listen
     * ============================== */

    if (listen(
            server_fd,
            TCP_SERVER_BACKLOG) < 0) {

        log_error(
            "listen() failed: %s",
            strerror(errno)
        );

        close(server_fd);

        return -1;
    }


    server->server_fd = server_fd;
    server->running = 1;

    log_info(
        "TCP server started at %s:%d",
        server->ip,
        server->port
    );


    /* ==============================
     * Accept Loop
     * ============================== */

    while (server->running) {

        struct sockaddr_in client_addr;
        socklen_t client_len =
            sizeof(client_addr);

        int client_fd = accept(
            server_fd,
            (struct sockaddr *)&client_addr,
            &client_len
        );

        if (client_fd < 0) {

            if (!server->running) {
                break;
            }

            if (errno == EINTR) {
                continue;
            }

            log_error(
                "accept() failed: %s",
                strerror(errno)
            );

            continue;
        }


        char client_ip[INET_ADDRSTRLEN];

        inet_ntop(
            AF_INET,
            &client_addr.sin_addr,
            client_ip,
            sizeof(client_ip)
        );

        log_info(
            "New client: %s:%d, fd=%d",
            client_ip,
            ntohs(client_addr.sin_port),
            client_fd
        );


        /* ==============================
         * Create Client Thread
         * ============================== */

        tcp_client_context_t *context =
            malloc(sizeof(tcp_client_context_t));

        if (context == NULL) {

            log_error(
                "Failed to allocate client context"
            );

            close(client_fd);

            continue;
        }

        context->server = server;
        context->client_fd = client_fd;


        pthread_t thread;

        int ret = pthread_create(
            &thread,
            NULL,
            tcp_client_thread,
            context
        );

        if (ret != 0) {

            log_error(
                "pthread_create() failed: %s",
                strerror(ret)
            );

            free(context);
            close(client_fd);

            continue;
        }


        pthread_detach(thread);
    }


    close(server_fd);

    server->server_fd = -1;

    log_info("TCP server stopped");

    return 0;
}


void tcp_server_stop(tcp_server_t *server)
{
    if (server == NULL) {
        return;
    }

    if (!server->running) {
        return;
    }

    server->running = 0;

    if (server->server_fd >= 0) {
        shutdown(
            server->server_fd,
            SHUT_RDWR
        );

        close(server->server_fd);

        server->server_fd = -1;
    }

    log_info("TCP server stop requested");
}


void tcp_server_destroy(tcp_server_t *server)
{
    if (server == NULL) {
        return;
    }

    if (server->running) {
        tcp_server_stop(server);
    }

    free(server);

    log_info("TCP server destroyed");
}

int tcp_server_send(
    int client_fd,
    const char *data,
    size_t length
)
{
    if (client_fd < 0 ||
        data == NULL ||
        length == 0) {
        return -1;
    }

    size_t total_sent = 0;

    while (total_sent < length) {

        ssize_t sent = send(
            client_fd,
            data + total_sent,
            length - total_sent,
            0
        );

        if (sent < 0) {

            if (errno == EINTR) {
                continue;
            }

            log_error(
                "send failed: fd=%d, error=%s",
                client_fd,
                strerror(errno)
            );

            return -1;
        }

        if (sent == 0) {
            return -1;
        }

        total_sent += (size_t)sent;
    }

    return 0;
}
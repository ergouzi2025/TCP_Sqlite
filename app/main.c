#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config/config.h"
#include "core/gateway.h"
#include "storage/sqlite/sqlite_db.h"
#include "transport/tcp/tcp_server.h"

#define OUTPUT_BUFFER_SIZE 4096

/**
 * @brief Send gateway output to TCP client.
 *
 * This callback is used by the gateway to
 * deliver normal responses and query batches
 * to the connected TCP client.
 */
static int gateway_output_handler(const char *data, size_t length, void *arg)
{
    int client_fd = *(int *)arg;

    if (data == NULL || length == 0) {
        return -1;
    }

    if (tcp_server_send(client_fd, data, length) != 0) {
        fprintf(stderr, "Failed to send gateway output: fd=%d\n", client_fd);

        return -1;
    }

    return 0;
}


/**
 * @brief Handle TCP client events.
 *
 * The TCP server reports connection,
 * received data, and disconnection events
 * through this callback.
 */
static int client_handler(int client_fd, tcp_client_event_t event, const char *data, size_t length, void *arg)
{
    gateway_t *gateway = (gateway_t *)arg;

    char input[BUFFER_SIZE];
    char output[OUTPUT_BUFFER_SIZE] = {0};

    if (gateway == NULL) {
        return -1;
    }


    if (event == TCP_CLIENT_CONNECTED) {
        const char *welcome = "TCP Sensor Gateway,Type HELP for available commands.\n";

        printf("Client connected: fd=%d\n", client_fd);

        if (tcp_server_send(client_fd, welcome, strlen(welcome)) != 0) {
            fprintf(stderr, "Failed to send welcome message: fd=%d\n", client_fd);

            return -1;
        }

        return 0;
    }


    if (event == TCP_CLIENT_DISCONNECTED) {
        printf("Client disconnected: fd=%d\n", client_fd);
        return 0;
    }


    if (event == TCP_CLIENT_DATA) {

        if (data == NULL || length == 0) {
            return -1;
        }

        /*
         * TCP data may not be terminated by '\0'.
         * Copy it into a local buffer and add '\0'.
         */
        if (length >= sizeof(input)) {
            fprintf(stderr, "Received data is too large\n");
            return -1;
        }

        memcpy(input, data, length);
        input[length] = '\0';

        printf("Received: %s\n", input);

        /*
         * Process command through gateway.
         *
         * SENSOR and HELP responses are written
         * into output.
         *
         * QUERY results are delivered batch by
         * batch through gateway_output_handler().
         */
        if (gateway_process(gateway, input, output, sizeof(output),
                    gateway_output_handler, &client_fd) != 0) {

            const char *error_response = "ERROR\n";

            if (tcp_server_send(client_fd, error_response, strlen(error_response)) != 0) {
                fprintf(stderr, "Failed to send error response: fd=%d\n", client_fd);
            }

            return -1;
        }

        /*
         * QUERY results have already been sent
         * by gateway_output_handler().
         *
         * SENSOR and HELP results remain in
         * the normal output buffer and are sent here.
         */
        if (output[0] != '\0') {
            if (tcp_server_send(client_fd, output, strlen(output)) != 0) {
                fprintf(stderr, "Failed to send response: fd=%d\n", client_fd);

                return -1;
            }
        }

        return 0;
    }


    return -1;
}

int main(int argc, char *argv[])
{
    const char *server_ip;
    int server_port;

    sqlite_db_t *db = NULL;
    gateway_t *gateway = NULL;
    tcp_server_t *server = NULL;

    /* Parse command-line arguments. */
    if (argc != 3) {

        printf("Usage: %s <ip> <port>\n", argv[0]);
        printf("Example: %s 192.168.171.132 8888\n", argv[0]);

        return EXIT_FAILURE;
    }

    server_ip = argv[1];
    server_port = atoi(argv[2]);

    if (server_port <= 0 || server_port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[2]);
        return EXIT_FAILURE;
    }


    /* Open the SQLite database. */
    db = sqlite_db_open(DB_FILE);

    if (db == NULL) {

        fprintf(stderr, "Failed to open database: %s\n", DB_FILE);

        return EXIT_FAILURE;
    }

    printf("Database opened: %s\n", DB_FILE);

    /* Create the gateway. */
    gateway = gateway_create(db);

    if (gateway == NULL) {
        fprintf(stderr, "Failed to create gateway\n");
        sqlite_db_close(db);
        return EXIT_FAILURE;
    }

    /* Create the TCP server. */
    server = tcp_server_create(server_ip, server_port);

    if (server == NULL) {

        fprintf(stderr, "Failed to create TCP server\n");

        gateway_destroy(gateway);
        sqlite_db_close(db);

        return EXIT_FAILURE;
    }


    /* Register the gateway as the TCP client handler. */
    if (tcp_server_set_handler(server, client_handler, gateway) != 0) {

        fprintf(stderr, "Failed to set TCP handler\n");

        tcp_server_destroy(server);
        gateway_destroy(gateway);
        sqlite_db_close(db);

        return EXIT_FAILURE;
    }


    printf("Server: %s:%d\n", server_ip, server_port);
    printf("Waiting for clients...\n");


    /* Start the server; this call blocks until the server stops. */
    if (tcp_server_start(server) != 0) {
        fprintf(stderr, "TCP server stopped with error\n");
    }


    /* Release resources in reverse creation order. */
    tcp_server_destroy(server);
    gateway_destroy(gateway);
    sqlite_db_close(db);

    printf("Server stopped.\n");

    return EXIT_SUCCESS;
}
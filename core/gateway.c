#include "gateway.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "protocol/tcp/tcp_parser.h"

struct gateway {
    sqlite_db_t *db;
    sensor_service_t *sensor_service;
    data_service_t *data_service;
};


/**
 * @brief Build HELP response.
 *
 * The current data ID range is obtained
 * from the data service and included in
 * the generated help information.
 *
 * @param gateway     Gateway pointer.
 * @param output      Output buffer.
 * @param output_size Output buffer size.
 *
 * @return 0 on success, -1 on failure.
 */
static int gateway_build_help(gateway_t *gateway, char *output, size_t output_size)
{
    int min_id;
    int max_id;

    if (gateway == NULL || output == NULL || output_size == 0) {
        return -1;
    }

    /*
     * Get current data ID range.
     */
    if (data_service_get_id_range(gateway->data_service, &min_id, &max_id) != 0) {
        return -1;
    }

    /*
     * Build HELP response.
     */
    snprintf(output, output_size,
        "TCP Sensor Gateway\n"
        "==================\n"
        "Commands:\n"
        "  SENSOR  Upload sensor data\n"
        "  QUERY   Query records\n"
        "  HELP    Show help\n"
        "\n"
        "Usage:\n"
        "  SENSOR,<device_id>,<data1>,<data2>,<data3>,<data4>\n"
        "  QUERY,<start_id>,<end_id>\n"
        "\n"
        "Example:\n"
        "  SENSOR,DEV2,26.8,55.2,101.3,568.4\n"
        "  QUERY,1,20\n"
        "\n"
        "Data Range: %d - %d\n"
        "==================\n",
        min_id,
        max_id);

    return 0;
}

/**
 * @brief Handle QUERY command batch by batch.
 *
 * The query result is not accumulated into one large
 * output buffer. Each batch is delivered immediately
 * through the output handler.
 *
 * @param gateway       Gateway pointer.
 * @param start_id      Query start ID.
 * @param end_id        Query end ID.
 * @param output_handler
 *                      Callback used to deliver each batch.
 * @param output_arg    User argument passed to output handler.
 *
 * @return 0 on success, -1 on failure.
 */
static int gateway_handle_query(gateway_t *gateway, int start_id, int end_id, gateway_output_handler_t output_handler, void *output_arg)
{
    int last_id = start_id - 1;
    int next_id;
    int row_count;
    int is_first_batch = 1;

    char batch_buffer[4096];

    while (1) {
        if (data_service_query_next_batch(gateway->data_service, last_id, end_id,
                                          batch_buffer, sizeof(batch_buffer),
                                          &next_id, &row_count, is_first_batch) != 0) {

            return -1;
        }

        if (row_count == 0) {
            break;
        }

        if (output_handler(batch_buffer, strlen(batch_buffer), output_arg) != 0) {

            return -1;
        }

        last_id = next_id;
        is_first_batch = 0;
    }

    return 0;
}

gateway_t *gateway_create(sqlite_db_t *db)
{
    gateway_t *gateway;

    if (db == NULL) {
        return NULL;
    }

    gateway = calloc(1, sizeof(*gateway));

    if (gateway == NULL) {
        return NULL;
    }

    gateway->db = db;

    gateway->sensor_service = sensor_service_create(db);

    if (gateway->sensor_service == NULL) {
        free(gateway);
        return NULL;
    }

    gateway->data_service = data_service_create(db);

    if (gateway->data_service == NULL) {
        sensor_service_destroy(gateway->sensor_service);
        free(gateway);
        return NULL;
    }

    return gateway;
}


void gateway_destroy(gateway_t *gateway)
{
    if (gateway == NULL) {
        return;
    }

    data_service_destroy(gateway->data_service);
    sensor_service_destroy(gateway->sensor_service);

    /* The caller owns the database and closes it separately. */

    free(gateway);
}


int gateway_process(gateway_t *gateway, const char *input, char *output, size_t output_size, gateway_output_handler_t output_handler, void *output_arg)
{
    tcp_command_t command;

    if (gateway == NULL || input == NULL || output == NULL || output_size == 0) {
        return -1;
    }

    output[0] = '\0';
    if (tcp_parser_parse(input, &command) != 0) {
        return -1;
    }

    switch (command.type) {

    case TCP_CMD_SENSOR_DATA:
        if (sensor_service_handle_data(gateway->sensor_service, &command.data.sensor) != 0) {

            return -1;
        }

        snprintf(output, output_size, "OK\n");

        return 0;


    case TCP_CMD_QUERY:
        if (output_handler == NULL) {
            return -1;
        }

        return gateway_handle_query(gateway, command.data.query.start_id,
                                    command.data.query.end_id, output_handler,
                                    output_arg);


    case TCP_CMD_HELP:
        return gateway_build_help(gateway, output, output_size);


    default:

        return -1;
    }
}
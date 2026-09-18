#ifndef TCP_PARSER_H
#define TCP_PARSER_H

#include <stddef.h>

/* ==============================
 * TCP Command Type
 * ============================== */

typedef enum {
    TCP_CMD_INVALID = 0,
    TCP_CMD_SENSOR_DATA,
    TCP_CMD_QUERY,
    TCP_CMD_HELP
} tcp_command_type_t;


/* ==============================
 * Sensor Data Command
 * ============================== */

typedef struct {
    char device_id[64];

    double data_1;
    double data_2;
    double data_3;
    double data_4;

} sensor_data_t;


/* ==============================
 * Query Command
 * ============================== */

typedef struct {
    int start_id;
    int end_id;

} query_command_t;


/* ==============================
 * TCP Command
 * ============================== */

typedef struct {
    tcp_command_type_t type;

    union {
        sensor_data_t sensor;
        query_command_t query;

    } data;

} tcp_command_t;


/* ==============================
 * Parser
 * ============================== */

/**
 * @brief Parse a TCP application command.
 *
 * Supported commands:
 *   SENSOR,...
 *   QUERY,...
 *   HELP
 *
 * @param input   Input command string.
 * @param command Parsed command output.
 *
 * @return 0 on success, -1 on failure.
 */
int tcp_parser_parse(
    const char *input,
    tcp_command_t *command
);

#endif
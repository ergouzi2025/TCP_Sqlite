#include "tcp_parser.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>


#define TCP_PARSER_MAX_FIELDS  8
#define TCP_PARSER_MAX_INPUT   256


/* ==============================
 * Internal Functions
 * ============================== */

static char *trim(char *str)
{
    char *end;

    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    end = str + strlen(str) - 1;

    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    *(end + 1) = '\0';

    return str;
}


static int split_fields(
    char *input,
    char **fields,
    int max_fields
)
{
    int count = 0;
    char *token;

    token = strtok(input, ",");

    while (token != NULL) {

        if (count >= max_fields) {
            return -1;
        }

        fields[count++] = trim(token);

        token = strtok(NULL, ",");
    }

    return count;
}


static int parse_double(
    const char *str,
    double *value
)
{
    char *end;
    double result;

    if (str == NULL || value == NULL) {
        return -1;
    }

    errno = 0;

    result = strtod(str, &end);

    if (str == end || *end != '\0' || errno != 0) {
        return -1;
    }

    *value = result;

    return 0;
}


static int parse_int(
    const char *str,
    int *value
)
{
    char *end;
    long result;

    if (str == NULL || value == NULL) {
        return -1;
    }

    errno = 0;

    result = strtol(str, &end, 10);

    if (str == end || *end != '\0' || errno != 0) {
        return -1;
    }

    if (result < 0 || result > 2147483647L) {
        return -1;
    }

    *value = (int)result;

    return 0;
}


/* ==============================
 * Parse SENSOR Command
 * ============================== */

static int parse_sensor(
    char **fields,
    int field_count,
    tcp_command_t *command
)
{
    if (field_count != 6) {
        return -1;
    }

    if (fields[1][0] == '\0') {
        return -1;
    }

    if (strlen(fields[1]) >= sizeof(command->data.sensor.device_id)) {
        return -1;
    }

    strcpy(
        command->data.sensor.device_id,
        fields[1]
    );

    if (parse_double(
            fields[2],
            &command->data.sensor.data_1) < 0) {
        return -1;
    }

    if (parse_double(
            fields[3],
            &command->data.sensor.data_2) < 0) {
        return -1;
    }

    if (parse_double(
            fields[4],
            &command->data.sensor.data_3) < 0) {
        return -1;
    }

    if (parse_double(
            fields[5],
            &command->data.sensor.data_4) < 0) {
        return -1;
    }

    command->type = TCP_CMD_SENSOR_DATA;

    return 0;
}


/* ==============================
 * Parse QUERY Command
 * ============================== */

static int parse_query(
    char **fields,
    int field_count,
    tcp_command_t *command
)
{
    int start_id;
    int end_id;

    if (field_count != 3) {
        return -1;
    }

    if (parse_int(fields[1], &start_id) < 0) {
        return -1;
    }

    if (parse_int(fields[2], &end_id) < 0) {
        return -1;
    }

    /*
     * Normalize reversed query range.
     *
     * QUERY,100,1
     * becomes
     * start_id = 1
     * end_id   = 100
     */
    if (start_id > end_id) {
        int temp = start_id;
        start_id = end_id;
        end_id = temp;
    }

    command->data.query.start_id = start_id;
    command->data.query.end_id = end_id;
    command->type = TCP_CMD_QUERY;

    return 0;
}


/* ==============================
 * Public Interface
 * ============================== */

int tcp_parser_parse(
    const char *input,
    tcp_command_t *command
)
{
    char buffer[TCP_PARSER_MAX_INPUT];
    char *fields[TCP_PARSER_MAX_FIELDS];
    int field_count;

    if (input == NULL || command == NULL) {
        return -1;
    }

    if (strlen(input) >= sizeof(buffer)) {
        return -1;
    }

    strcpy(buffer, input);

    memset(command, 0, sizeof(*command));

    field_count = split_fields(
        buffer,
        fields,
        TCP_PARSER_MAX_FIELDS
    );

    if (field_count <= 0) {
        return -1;
    }

    if (strcmp(fields[0], "SENSOR") == 0) {

        return parse_sensor(
            fields,
            field_count,
            command
        );
    }

    if (strcmp(fields[0], "QUERY") == 0) {

        return parse_query(
            fields,
            field_count,
            command
        );
    }

    if (strcmp(fields[0], "HELP") == 0) {

        if (field_count != 1) {
            return -1;
        }

        command->type = TCP_CMD_HELP;

        return 0;
    }

    return -1;
}
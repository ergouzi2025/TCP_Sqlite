#ifndef GATEWAY_H
#define GATEWAY_H

#include <stddef.h>

#include "storage/sqlite/sqlite_db.h"
#include "service/sensor_service.h"
#include "service/data_service.h"

typedef struct gateway gateway_t;

/**
 * @brief Output callback used by gateway.
 *
 * The callback is called when the gateway
 * has a response or a query result batch
 * that should be delivered to the upper layer.
 *
 * @param data   Output data.
 * @param length Output data length.
 * @param arg    User-defined argument.
 *
 * @return 0 on success, -1 on failure.
 */
typedef int (*gateway_output_handler_t)(const char *data, size_t length, void *arg);


/**
 * @brief Create gateway.
 *
 * @param db SQLite database handle.
 *
 * @return Gateway pointer on success,
 *         NULL on failure.
 */
gateway_t *gateway_create(sqlite_db_t *db);


/**
 * @brief Destroy gateway.
 *
 * The database handle is not closed here.
 *
 * @param gateway Gateway pointer.
 */
void gateway_destroy(gateway_t *gateway);


/**
 * @brief Process one TCP command.
 *
 * The gateway parses the input command and
 * dispatches it to the corresponding service.
 *
 * Supported commands:
 *
 * SENSOR,<device_id>,<data1>,<data2>,<data3>,<data4>
 * QUERY,<start_id>,<end_id>
 * HELP
 *
 * QUERY results are processed batch by batch
 * and delivered through the output handler.
 *
 * @param gateway       Gateway pointer.
 * @param input         Input command string.
 * @param output        Output buffer for normal responses.
 * @param output_size   Output buffer size.
 * @param output_handler
 *                      Callback used to deliver query
 *                      result batches.
 * @param output_arg    User argument passed to output handler.
 *
 * @return 0 on success, -1 on failure.
 */
int gateway_process(gateway_t *gateway, const char *input, char *output, size_t output_size, gateway_output_handler_t output_handler, void *output_arg);

#endif
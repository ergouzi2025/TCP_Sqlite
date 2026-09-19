#ifndef DATA_SERVICE_H
#define DATA_SERVICE_H

#include <stddef.h>

#include "storage/sqlite/sqlite_db.h"

typedef struct data_service data_service_t;

data_service_t *data_service_create(sqlite_db_t *db);

void data_service_destroy(data_service_t *service);

/**
 * @brief Get the current sensor data ID range.
 *
 * @param service Data service instance.
 * @param min_id  Minimum available ID.
 * @param max_id  Maximum available ID.
 *
 * @return 0 on success, -1 on failure.
 */
int data_service_get_id_range(data_service_t *service, int *min_id, int *max_id);

/**
 * @brief Query the next batch of sensor data.
 *
 * Records are queried in ascending ID order.
 * Each query returns at most
 * SQLITE_DB_QUERY_BATCH_SIZE records.
 *
 * @param service       Data service instance.
 * @param last_id       Last ID returned by the previous query.
 * @param end_id        Maximum ID to query.
 * @param buffer        Output buffer.
 * @param buffer_size   Output buffer size.
 * @param next_id       ID of the last record returned.
 * @param row_count     Number of records returned.
 * @param is_first_batch
 *                       Non-zero to include the table header.
 *                       Zero to output data rows only.
 *
 * @return 0 on success, -1 on failure.
 */
int data_service_query_next_batch(data_service_t *service, int last_id, int end_id, char *buffer, size_t buffer_size, int *next_id, int *row_count, int is_first_batch);
#endif
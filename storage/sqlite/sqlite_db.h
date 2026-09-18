#ifndef SQLITE_DB_H
#define SQLITE_DB_H

#include <stddef.h>

/*
 * Maximum number of records returned by one batch query.
 *
 * This value can be changed according to the required
 * output buffer size.
 */
#define SQLITE_DB_QUERY_BATCH_SIZE 20


/*
 * SQLite database context.
 *
 * The actual sqlite3 handle is hidden inside sqlite_db.c.
 */
typedef struct sqlite_db sqlite_db_t;


/*
 * Open or create a SQLite database.
 *
 * If the database file does not exist, SQLite will create it.
 *
 * This function also:
 *   - configures busy timeout
 *   - enables WAL mode
 *   - creates required tables
 *   - creates required indexes
 *
 * Return:
 *   Pointer to sqlite_db_t on success
 *   NULL on failure
 */
sqlite_db_t *sqlite_db_open(const char *db_path);


/*
 * Insert one sensor data record.
 *
 * timestamp is not passed here.
 * SQLite automatically generates it using:
 *
 * datetime('now', 'localtime')
 *
 * Return:
 *   0  success
 *  -1  failure
 */
int sqlite_db_insert_sensor(
    sqlite_db_t *db,
    const char *device_id,
    double data_1,
    double data_2,
    double data_3,
    double data_4
);


/*
 * Get the minimum and maximum record ID.
 *
 * If the table is empty:
 *   min_id = 0
 *   max_id = 0
 *
 * Return:
 *   0  success
 *  -1  failure
 */
int sqlite_db_get_id_range(
    sqlite_db_t *db,
    int *min_id,
    int *max_id
);


/*
 * Query one batch of sensor records.
 *
 * Records are queried in ascending ID order.
 *
 * The query starts after last_id and does not exceed end_id.
 * At most SQLITE_DB_QUERY_BATCH_SIZE records are returned.
 *
 * next_id:
 *   ID of the last record returned in this batch.
 *
 * row_count:
 *   Number of records actually returned.
 *   0 means no more records are available.
 *
 * Return:
 *   0  success
 *  -1  failure
 */
int sqlite_db_query_next_batch(
    sqlite_db_t *db,
    int last_id,
    int end_id,
    char *buffer,
    size_t buffer_size,
    int *next_id,
    int *row_count,
    int is_first_batch
);


/*
 * Close the database and release resources.
 */
void sqlite_db_close(sqlite_db_t *db);


#endif /* SQLITE_DB_H */
#include "sqlite_db.h"

#include <sqlite3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/log/log.h"


/*
 * Private database context.
 *
 * sqlite3 handle is intentionally hidden from other modules.
 */
struct sqlite_db {
    sqlite3 *handle;
};


/*
 * Database schema.
 *
 * This function is private to sqlite_db.c.
 *
 * Upper-layer modules do not need to know
 * how the database tables are created.
 */
static int sqlite_db_init_schema(sqlite_db_t *db)
{
    static const char *schema_sql =
        "CREATE TABLE IF NOT EXISTS sensor_data ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    device_id TEXT NOT NULL,"
        "    data_1 REAL,"
        "    data_2 REAL,"
        "    data_3 REAL,"
        "    data_4 REAL,"
        "    timestamp DATETIME DEFAULT (datetime('now', 'localtime'))"
        ");"

        "CREATE INDEX IF NOT EXISTS idx_sensor_data_device_id "
        "ON sensor_data(device_id);";

    char *error_message = NULL;

    int rc = sqlite3_exec(
        db->handle,
        schema_sql,
        NULL,
        NULL,
        &error_message
    );

    if (rc != SQLITE_OK) {

        log_error(
            "Failed to initialize database schema: %s",
            error_message ? error_message : "unknown error"
        );

        sqlite3_free(error_message);

        return -1;
    }

    log_info("Database schema initialized successfully");

    return 0;
}


/*
 * Open or create database.
 */
sqlite_db_t *sqlite_db_open(const char *db_path)
{
    sqlite_db_t *db = NULL;

    if (db_path == NULL || db_path[0] == '\0') {
        log_error("Invalid database path");

        return NULL;
    }


    /*
     * Allocate database context.
     */
    db = malloc(sizeof(*db));

    if (db == NULL) {
        log_error("Failed to allocate sqlite_db_t");

        return NULL;
    }

    db->handle = NULL;


    /*
     * Open database.
     *
     * If the database file does not exist,
     * sqlite3_open() creates it automatically.
     */
    int rc = sqlite3_open(db_path, &db->handle);

    if (rc != SQLITE_OK) {

        log_error(
            "Failed to open database '%s': %s",
            db_path,
            db->handle ? sqlite3_errmsg(db->handle) : "unknown error"
        );

        if (db->handle != NULL) {
            sqlite3_close(db->handle);
        }

        free(db);

        return NULL;
    }


    /*
     * Configure SQLite busy timeout.
     *
     * If the database is temporarily locked,
     * SQLite will wait up to 1000 ms.
     */
    rc = sqlite3_busy_timeout(db->handle, 1000);

    if (rc != SQLITE_OK) {

        log_error(
            "Failed to configure SQLite busy timeout: %s",
            sqlite3_errmsg(db->handle)
        );

        sqlite3_close(db->handle);
        free(db);

        return NULL;
    }


    /*
     * Enable WAL mode.
     *
     * WAL improves concurrent read/write behavior
     * and is suitable for the gateway architecture.
     */
    char *error_message = NULL;

    rc = sqlite3_exec(
        db->handle,
        "PRAGMA journal_mode=WAL;",
        NULL,
        NULL,
        &error_message
    );

    if (rc != SQLITE_OK) {

        log_error(
            "Failed to enable WAL mode: %s",
            error_message ? error_message : "unknown error"
        );

        sqlite3_free(error_message);

        sqlite3_close(db->handle);
        free(db);

        return NULL;
    }


    /*
     * Initialize database schema.
     *
     * This automatically creates:
     *
     *   sensor_data
     *
     * and:
     *
     *   idx_sensor_data_device_id
     */
    if (sqlite_db_init_schema(db) < 0) {

        log_error("Failed to initialize database schema");

        sqlite3_close(db->handle);
        free(db);

        return NULL;
    }


    log_info("SQLite database opened: %s", db_path);

    return db;
}


/*
 * Insert sensor data.
 */
int sqlite_db_insert_sensor(
    sqlite_db_t *db,
    const char *device_id,
    double data_1,
    double data_2,
    double data_3,
    double data_4
)
{
    static const char *sql =
        "INSERT INTO sensor_data "
        "(device_id, data_1, data_2, data_3, data_4) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt = NULL;


    /*
     * Validate parameters.
     */
    if (db == NULL ||
        db->handle == NULL ||
        device_id == NULL ||
        device_id[0] == '\0') {

        log_error("Invalid parameters for sqlite_db_insert_sensor");

        return -1;
    }


    /*
     * Prepare SQL statement.
     */
    int rc = sqlite3_prepare_v2(
        db->handle,
        sql,
        -1,
        &stmt,
        NULL
    );

    if (rc != SQLITE_OK) {

        log_error(
            "Failed to prepare insert statement: %s",
            sqlite3_errmsg(db->handle)
        );

        return -1;
    }


    /*
     * Bind parameters.
     *
     * Parameter order:
     *
     *   1 -> device_id
     *   2 -> data_1
     *   3 -> data_2
     *   4 -> data_3
     *   5 -> data_4
     */
    rc = sqlite3_bind_text(
        stmt,
        1,
        device_id,
        -1,
        SQLITE_TRANSIENT
    );

    if (rc != SQLITE_OK) {
        log_error("Failed to bind device_id");
        sqlite3_finalize(stmt);
        return -1;
    }


    rc = sqlite3_bind_double(stmt, 2, data_1);

    if (rc != SQLITE_OK) {
        log_error("Failed to bind data_1");
        sqlite3_finalize(stmt);
        return -1;
    }


    rc = sqlite3_bind_double(stmt, 3, data_2);

    if (rc != SQLITE_OK) {
        log_error("Failed to bind data_2");
        sqlite3_finalize(stmt);
        return -1;
    }


    rc = sqlite3_bind_double(stmt, 4, data_3);

    if (rc != SQLITE_OK) {
        log_error("Failed to bind data_3");
        sqlite3_finalize(stmt);
        return -1;
    }


    rc = sqlite3_bind_double(stmt, 5, data_4);

    if (rc != SQLITE_OK) {
        log_error("Failed to bind data_4");
        sqlite3_finalize(stmt);
        return -1;
    }


    /*
     * Execute INSERT.
     */
    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {

        log_error(
            "Failed to insert sensor data: %s",
            sqlite3_errmsg(db->handle)
        );

        sqlite3_finalize(stmt);

        return -1;
    }


    /*
     * Release prepared statement.
     */
    sqlite3_finalize(stmt);

    // log_debug(
    //     "Sensor data inserted: device=%s",
    //     device_id
    // );

    return 0;
}


/*
 * Get minimum and maximum record ID.
 */
int sqlite_db_get_id_range(
    sqlite_db_t *db,
    int *min_id,
    int *max_id
)
{
    static const char *sql =
        "SELECT MIN(id), MAX(id) "
        "FROM sensor_data;";

    sqlite3_stmt *stmt = NULL;


    if (db == NULL ||
        db->handle == NULL ||
        min_id == NULL ||
        max_id == NULL) {

        log_error("Invalid parameters for sqlite_db_get_id_range");

        return -1;
    }


    /*
     * Default result for an empty table.
     */
    *min_id = 0;
    *max_id = 0;


    /*
     * Prepare statement.
     */
    int rc = sqlite3_prepare_v2(
        db->handle,
        sql,
        -1,
        &stmt,
        NULL
    );

    if (rc != SQLITE_OK) {

        log_error(
            "Failed to prepare ID range query: %s",
            sqlite3_errmsg(db->handle)
        );

        return -1;
    }


    /*
     * Execute query.
     */
    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {

        log_error(
            "Failed to query ID range: %s",
            sqlite3_errmsg(db->handle)
        );

        sqlite3_finalize(stmt);

        return -1;
    }


    /*
     * MIN(id) / MAX(id) return NULL
     * when the table is empty.
     */
    if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
        *min_id = sqlite3_column_int(stmt, 0);
    }

    if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
        *max_id = sqlite3_column_int(stmt, 1);
    }


    sqlite3_finalize(stmt);

    return 0;
}


/*
 * Query one batch of sensor data.
 *
 * Records are queried in ascending ID order.
 * At most SQLITE_DB_QUERY_BATCH_SIZE records are returned.
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
)
{
    static const char *sql =
        "SELECT "
        "id, "
        "device_id, "
        "data_1, "
        "data_2, "
        "data_3, "
        "data_4, "
        "timestamp "
        "FROM sensor_data "
        "WHERE id > ? "
        "AND id <= ? "
        "ORDER BY id ASC "
        "LIMIT ?;";

    sqlite3_stmt *stmt = NULL;

    size_t used = 0;

    int count = 0;
    int current_id = last_id;


    /*
     * Validate parameters.
     */
    if (db == NULL ||
        db->handle == NULL ||
        buffer == NULL ||
        buffer_size == 0 ||
        next_id == NULL ||
        row_count == NULL) {

        log_error(
            "Invalid parameters for sqlite_db_query_next_batch"
        );

        return -1;
    }


    /*
     * Initialize output values.
     */
    *next_id = last_id;
    *row_count = 0;

    buffer[0] = '\0';


    /*
     * Prepare SQL statement.
     */
    int rc = sqlite3_prepare_v2(
        db->handle,
        sql,
        -1,
        &stmt,
        NULL
    );

    if (rc != SQLITE_OK) {

        log_error(
            "Failed to prepare batch query: %s",
            sqlite3_errmsg(db->handle)
        );

        return -1;
    }


    /*
     * Bind last_id.
     *
     * Only records with ID greater than last_id
     * will be returned.
     */
    rc = sqlite3_bind_int(
        stmt,
        1,
        last_id
    );

    if (rc != SQLITE_OK) {

        log_error("Failed to bind last_id");

        sqlite3_finalize(stmt);

        return -1;
    }


    /*
     * Bind end_id.
     */
    rc = sqlite3_bind_int(
        stmt,
        2,
        end_id
    );

    if (rc != SQLITE_OK) {

        log_error("Failed to bind end_id");

        sqlite3_finalize(stmt);

        return -1;
    }


    /*
     * Limit the number of records returned
     * by one query.
     */
    rc = sqlite3_bind_int(
        stmt,
        3,
        SQLITE_DB_QUERY_BATCH_SIZE
    );

    if (rc != SQLITE_OK) {

        log_error("Failed to bind query batch size");

        sqlite3_finalize(stmt);

        return -1;
    }


    /*
     * Output table header only for the first batch.
     */
    if (is_first_batch) {

        int written = snprintf(
            buffer + used,
            buffer_size - used,
            "id | device_id | data_1 | data_2 | data_3 | data_4 | timestamp\n"
        );

        if (written < 0 ||
            (size_t)written >= buffer_size - used) {

            log_error("Query result buffer is too small");

            sqlite3_finalize(stmt);

            buffer[buffer_size - 1] = '\0';

            return -1;
        }

        used += (size_t)written;
    }


    /*
     * Read query results row by row.
     */
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {

        int id =
            sqlite3_column_int(stmt, 0);

        const unsigned char *device_id =
            sqlite3_column_text(stmt, 1);

        double data_1 =
            sqlite3_column_double(stmt, 2);

        double data_2 =
            sqlite3_column_double(stmt, 3);

        double data_3 =
            sqlite3_column_double(stmt, 4);

        double data_4 =
            sqlite3_column_double(stmt, 5);

        const unsigned char *timestamp =
            sqlite3_column_text(stmt, 6);


        /*
         * sqlite3_column_text() can theoretically return NULL.
         */
        if (device_id == NULL) {
            device_id = (const unsigned char *)"";
        }

        if (timestamp == NULL) {
            timestamp = (const unsigned char *)"";
        }


        /*
         * Format one record into the output buffer.
         */
        int written = snprintf(
            buffer + used,
            buffer_size - used,
            "%d | %s | %.6f | %.6f | %.6f | %.6f | %s\n",
            id,
            device_id,
            data_1,
            data_2,
            data_3,
            data_4,
            timestamp
        );


        if (written < 0 ||
            (size_t)written >= buffer_size - used) {

            log_error("Query result buffer is too small");

            sqlite3_finalize(stmt);

            buffer[buffer_size - 1] = '\0';

            return -1;
        }


        used += (size_t)written;

        count++;
        current_id = id;
    }


    /*
     * SQLITE_DONE means the query completed normally.
     */
    if (rc != SQLITE_DONE) {

        log_error(
            "Failed while querying sensor data: %s",
            sqlite3_errmsg(db->handle)
        );

        sqlite3_finalize(stmt);

        return -1;
    }


    sqlite3_finalize(stmt);


    /*
     * Return batch information.
     */
    *next_id = current_id;
    *row_count = count;

    return 0;
}

/*
 * Close database.
 */
void sqlite_db_close(sqlite_db_t *db)
{
    if (db == NULL) {
        return;
    }


    if (db->handle != NULL) {

        sqlite3_close(db->handle);

        db->handle = NULL;
    }


    free(db);

    log_info("SQLite database closed");
}
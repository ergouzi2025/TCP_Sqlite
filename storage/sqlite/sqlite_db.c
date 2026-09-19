#include "sqlite_db.h"

#include <sqlite3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/log/log.h"

/* Keep the SQLite handle private to this module. */
struct sqlite_db {
    sqlite3 *handle;
};

/* Initialize tables and indexes required by the gateway. */
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

    int rc = sqlite3_exec(db->handle, schema_sql, NULL, NULL, &error_message);

    if (rc != SQLITE_OK) {

        log_error("Failed to initialize database schema: %s",
                  error_message ? error_message : "unknown error");

        sqlite3_free(error_message);

        return -1;
    }

    log_info("Database schema initialized successfully");

    return 0;
}


/* Open or create the database. */
sqlite_db_t *sqlite_db_open(const char *db_path)
{
    sqlite_db_t *db = NULL;

    if (db_path == NULL || db_path[0] == '\0') {
        log_error("Invalid database path");

        return NULL;
    }


    db = calloc(1, sizeof(*db));

    if (db == NULL) {
        log_error("Failed to allocate sqlite_db_t");

        return NULL;
    }

    /* sqlite3_open creates the file when it does not exist. */
    int rc = sqlite3_open(db_path, &db->handle);

    if (rc != SQLITE_OK) {

        log_error("Failed to open database '%s': %s", db_path,
                  db->handle ? sqlite3_errmsg(db->handle) : "unknown error");

        if (db->handle != NULL) {
            sqlite3_close(db->handle);
        }

        free(db);

        return NULL;
    }


    /* Wait briefly when another connection holds a lock. */
    rc = sqlite3_busy_timeout(db->handle, 1000);

    if (rc != SQLITE_OK) {

        log_error("Failed to configure SQLite busy timeout: %s",
                  sqlite3_errmsg(db->handle));

        sqlite3_close(db->handle);
        free(db);

        return NULL;
    }


    /* WAL improves concurrent read/write behavior. */
    char *error_message = NULL;

    rc = sqlite3_exec(db->handle, "PRAGMA journal_mode=WAL;", NULL, NULL,
                      &error_message);

    if (rc != SQLITE_OK) {

        log_error("Failed to enable WAL mode: %s",
                  error_message ? error_message : "unknown error");

        sqlite3_free(error_message);

        sqlite3_close(db->handle);
        free(db);

        return NULL;
    }


    /* Create the table and index required by the gateway. */
    if (sqlite_db_init_schema(db) < 0) {

        log_error("Failed to initialize database schema");

        sqlite3_close(db->handle);
        free(db);

        return NULL;
    }


    log_info("SQLite database opened: %s", db_path);

    return db;
}


int sqlite_db_insert_sensor(sqlite_db_t *db, const char *device_id, double data_1, double data_2, double data_3, double data_4)
{
    static const char *sql =
        "INSERT INTO sensor_data "
        "(device_id, data_1, data_2, data_3, data_4) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt = NULL;


    if (db == NULL ||
        db->handle == NULL ||
        device_id == NULL ||
        device_id[0] == '\0') {

        log_error("Invalid parameters for sqlite_db_insert_sensor");

        return -1;
    }


    int rc = sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {

        log_error("Failed to prepare insert statement: %s",
                  sqlite3_errmsg(db->handle));

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
    rc = sqlite3_bind_text(stmt, 1, device_id, -1, SQLITE_TRANSIENT);

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


    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {

        log_error("Failed to insert sensor data: %s",
                  sqlite3_errmsg(db->handle));

        sqlite3_finalize(stmt);

        return -1;
    }


    sqlite3_finalize(stmt);

    // log_debug(
    //     "Sensor data inserted: device=%s",
    //     device_id
    // );

    return 0;
}


int sqlite_db_get_id_range(sqlite_db_t *db, int *min_id, int *max_id)
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


    *min_id = 0;
    *max_id = 0;


    int rc = sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {

        log_error("Failed to prepare ID range query: %s",
                  sqlite3_errmsg(db->handle));

        return -1;
    }


    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {

        log_error("Failed to query ID range: %s", sqlite3_errmsg(db->handle));

        sqlite3_finalize(stmt);

        return -1;
    }


    /* MIN/MAX return NULL when the table is empty. */
    if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
        *min_id = sqlite3_column_int(stmt, 0);
    }

    if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
        *max_id = sqlite3_column_int(stmt, 1);
    }


    sqlite3_finalize(stmt);

    return 0;
}


/* Query one ascending-ID batch after last_id and through end_id. */
int sqlite_db_query_next_batch(sqlite_db_t *db, int last_id, int end_id, char *buffer, size_t buffer_size, int *next_id, int *row_count, int is_first_batch)
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


    *next_id = last_id;
    *row_count = 0;

    buffer[0] = '\0';


    int rc = sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {

        log_error("Failed to prepare batch query: %s",
                  sqlite3_errmsg(db->handle));

        return -1;
    }


    rc = sqlite3_bind_int(stmt, 1, last_id);

    if (rc != SQLITE_OK) {

        log_error("Failed to bind last_id");

        sqlite3_finalize(stmt);

        return -1;
    }


    rc = sqlite3_bind_int(stmt, 2, end_id);

    if (rc != SQLITE_OK) {

        log_error("Failed to bind end_id");

        sqlite3_finalize(stmt);

        return -1;
    }


    rc = sqlite3_bind_int(stmt, 3, SQLITE_DB_QUERY_BATCH_SIZE);

    if (rc != SQLITE_OK) {

        log_error("Failed to bind query batch size");

        sqlite3_finalize(stmt);

        return -1;
    }


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


        /* SQLite text columns may be NULL. */
        if (device_id == NULL) {
            device_id = (const unsigned char *)"";
        }

        if (timestamp == NULL) {
            timestamp = (const unsigned char *)"";
        }


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


    if (rc != SQLITE_DONE) {

        log_error("Failed while querying sensor data: %s",
                  sqlite3_errmsg(db->handle));

        sqlite3_finalize(stmt);

        return -1;
    }


    sqlite3_finalize(stmt);


    *next_id = current_id;
    *row_count = count;

    return 0;
}

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
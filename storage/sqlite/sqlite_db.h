#ifndef SQLITE_DB_H
#define SQLITE_DB_H

#include <stddef.h>

/* Maximum number of records returned by one batch query. */
#define SQLITE_DB_QUERY_BATCH_SIZE 20


/* The SQLite handle is private to sqlite_db.c. */
typedef struct sqlite_db sqlite_db_t;


/* Open or create a database and initialize its schema. */
sqlite_db_t *sqlite_db_open(const char *db_path);


/* Insert one sensor record; SQLite generates its timestamp. */
int sqlite_db_insert_sensor(sqlite_db_t *db, const char *device_id, double data_1, double data_2, double data_3, double data_4);


/* Get the minimum and maximum record IDs. Empty tables return zeroes. */
int sqlite_db_get_id_range(sqlite_db_t *db, int *min_id, int *max_id);


/* Query one ascending-ID batch after last_id and through end_id. */
int sqlite_db_query_next_batch(sqlite_db_t *db, int last_id, int end_id, char *buffer, size_t buffer_size, int *next_id, int *row_count, int is_first_batch);


/* Close the database and release its resources. */
void sqlite_db_close(sqlite_db_t *db);


#endif /* SQLITE_DB_H */
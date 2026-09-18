#include "data_service.h"

#include <stdlib.h>


/* ==============================
 * Data Service Structure
 * ============================== */

struct data_service {
    sqlite_db_t *db;
};


/* ==============================
 * Create
 * ============================== */

data_service_t *data_service_create(
    sqlite_db_t *db
)
{
    data_service_t *service;

    if (db == NULL) {
        return NULL;
    }

    service = calloc(1, sizeof(data_service_t));

    if (service == NULL) {
        return NULL;
    }

    service->db = db;

    return service;
}


/* ==============================
 * Get ID Range
 * ============================== */

int data_service_get_id_range(
    data_service_t *service,
    int *min_id,
    int *max_id
)
{
    if (service == NULL ||
        service->db == NULL ||
        min_id == NULL ||
        max_id == NULL) {

        return -1;
    }

    return sqlite_db_get_id_range(
        service->db,
        min_id,
        max_id
    );
}

/* ==============================
 * Query Data
 * ============================== */

int data_service_query_next_batch(
    data_service_t *service,
    int last_id,
    int end_id,
    char *buffer,
    size_t buffer_size,
    int *next_id,
    int *row_count,
    int is_first_batch
)
{
    if (service == NULL ||
        service->db == NULL ||
        buffer == NULL ||
        buffer_size == 0 ||
        next_id == NULL ||
        row_count == NULL) {

        return -1;
    }

    return sqlite_db_query_next_batch(
        service->db,
        last_id,
        end_id,
        buffer,
        buffer_size,
        next_id,
        row_count,
        is_first_batch
    );
}

/* ==============================
 * Destroy
 * ============================== */

void data_service_destroy(
    data_service_t *service
)
{
    if (service == NULL) {
        return;
    }

    free(service);
}
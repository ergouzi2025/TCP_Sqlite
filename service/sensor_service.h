#ifndef SENSOR_SERVICE_H
#define SENSOR_SERVICE_H

#include "protocol/tcp/tcp_parser.h"
#include "storage/sqlite/sqlite_db.h"

typedef struct sensor_service sensor_service_t;


/* ==============================
 * Create / Destroy
 * ============================== */

sensor_service_t *sensor_service_create(
    sqlite_db_t *db
);

void sensor_service_destroy(
    sensor_service_t *service
);


/* ==============================
 * Handle Sensor Data
 * ============================== */

int sensor_service_handle_data(
    sensor_service_t *service,
    const sensor_data_t *data
);

#endif
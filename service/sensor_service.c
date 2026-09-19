#include "sensor_service.h"

#include "utils/log/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct sensor_service {
    sqlite_db_t *db;
};


sensor_service_t *sensor_service_create(sqlite_db_t *db)
{
    if (db == NULL) {
        log_error("sensor_service_create: invalid database");

        return NULL;
    }

    sensor_service_t *service = calloc(1, sizeof(*service));

    if (service == NULL) {
        log_error(
            "Failed to allocate sensor service"
        );

        return NULL;
    }

    service->db = db;
    log_info("Sensor service created");
    return service;
}


int sensor_service_handle_data(sensor_service_t *service, const sensor_data_t *data)
{
    if (service == NULL || service->db == NULL || data == NULL) {
        log_error("sensor_service_handle_data: invalid parameter");

        return -1;
    }


    if (data->device_id[0] == '\0') {
        log_error("Sensor data has empty device ID");

        return -1;
    }


    int ret = sqlite_db_insert_sensor(service->db, data->device_id, data->data_1,
                                      data->data_2, data->data_3, data->data_4);

    if (ret < 0) {

        log_error("Failed to store sensor data: device=%s", data->device_id);

        return -1;
    }


    log_info("Sensor data stored: device=%s", data->device_id);

    return 0;
}


void sensor_service_destroy(sensor_service_t *service)
{
    if (service == NULL) {
        return;
    }


    free(service);

    log_info("Sensor service destroyed");
}
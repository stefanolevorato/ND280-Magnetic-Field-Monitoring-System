#ifndef MEASUREMENT_SERVICE_H
#define MEASUREMENT_SERVICE_H

#include <stdint.h>
#include "calibration.h"
#include "tmag5273.h"

typedef struct
{
    tmag5273_measurement_t value;
    uint8_t valid_samples;
} measurement_result_t;

uint8_t measurement_service_acquire_average(
    const calibration_record_t *calibration,
    measurement_result_t *result,
    void (*background_process)(void));

#endif

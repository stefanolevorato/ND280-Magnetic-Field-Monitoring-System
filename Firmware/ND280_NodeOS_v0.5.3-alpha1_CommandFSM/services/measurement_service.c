#include "config.h"
#include <stdint.h>
#include <util/delay.h>

#include "measurement_service.h"

uint8_t measurement_service_acquire_average(
    const calibration_record_t *calibration,
    measurement_result_t *result,
    void (*background_process)(void))
{
    int32_t sum_temperature = 0;
    int32_t sum_bx = 0;
    int32_t sum_by = 0;
    int32_t sum_bz = 0;
    uint8_t last_status = 0;
    uint8_t valid = 0;

    if (calibration == 0 || result == 0) return 0U;

    for (uint8_t index = 0; index < AVERAGE_SAMPLES; index++) {
        tmag5273_raw_sample_t sample;

        if (tmag5273_read_raw(&sample)) {
            sum_temperature += sample.temperature_raw;
            sum_bx += sample.bx_raw;
            sum_by += sample.by_raw;
            sum_bz += sample.bz_raw;
            last_status = sample.status;
            valid++;
        }

        if (background_process != 0) background_process();
        _delay_ms(SAMPLE_INTERVAL_MS);
    }

    result->valid_samples = valid;
    if (valid == 0U) return 0U;

    tmag5273_convert_averaged_raw(sum_temperature / valid,
                                  sum_bx / valid,
                                  sum_by / valid,
                                  sum_bz / valid,
                                  last_status,
                                  &result->value);

    calibration_apply(calibration, &result->value);
    return 1U;
}

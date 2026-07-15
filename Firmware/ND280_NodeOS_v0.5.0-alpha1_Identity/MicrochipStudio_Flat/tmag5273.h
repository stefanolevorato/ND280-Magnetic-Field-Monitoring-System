#ifndef TMAG5273_H
#define TMAG5273_H

#include <stdint.h>

typedef struct
{
    int16_t temperature_raw;
    int16_t bx_raw;
    int16_t by_raw;
    int16_t bz_raw;
    uint8_t status;
} tmag5273_raw_sample_t;

typedef struct
{
    int32_t temperature_x100;
    int32_t bx_x100;
    int32_t by_x100;
    int32_t bz_x100;
    uint8_t status;
} tmag5273_measurement_t;

uint8_t tmag5273_read_device_id(uint8_t *device_id);
uint8_t tmag5273_init(void);
uint8_t tmag5273_read_raw(tmag5273_raw_sample_t *sample);
void tmag5273_convert_averaged_raw(int32_t temperature_raw,
                                  int32_t bx_raw,
                                  int32_t by_raw,
                                  int32_t bz_raw,
                                  uint8_t status,
                                  tmag5273_measurement_t *measurement);

#endif

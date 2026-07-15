#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <stdint.h>
#include "tmag5273.h"

#define CALIBRATION_MAGIC   0x4E443238UL  /* ASCII-like marker: "ND28" */
#define CALIBRATION_VERSION 1U
#define CALIBRATION_GAIN_UNITY_PPM 1000000L

typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t size;

    /* Offsets in hundredths of mT, matching the firmware measurement format. */
    int32_t offset_x_x100;
    int32_t offset_y_x100;
    int32_t offset_z_x100;

    /* Multiplicative gain in parts per million; 1,000,000 = unity. */
    int32_t gain_x_ppm;
    int32_t gain_y_ppm;
    int32_t gain_z_ppm;

    /* Noise sigma in 0.0001 mT units, preserving GUI characterization precision. */
    uint32_t noise_sigma_x_x10000;
    uint32_t noise_sigma_y_x10000;
    uint32_t noise_sigma_z_x10000;

    /* Temperature at calibration in hundredths of a degree Celsius. */
    int32_t calibration_temperature_x100;

    uint32_t calibration_counter;
    uint16_t flags;
    uint16_t crc;
} calibration_record_t;

typedef enum
{
    CALIBRATION_STATUS_DEFAULT = 0,
    CALIBRATION_STATUS_VALID = 1
} calibration_status_t;

void calibration_make_defaults(calibration_record_t *record);
calibration_status_t calibration_load(calibration_record_t *record);
uint8_t calibration_save(const calibration_record_t *record);
void calibration_factory_reset(void);
void calibration_apply(const calibration_record_t *record,
                       tmag5273_measurement_t *measurement);

#endif

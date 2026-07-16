#include <avr/eeprom.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "calibration.h"
#include "crc16.h"
#include "diagnostics.h"

static calibration_record_t EEMEM eeprom_calibration;

static uint16_t calibration_crc(const calibration_record_t *record)
{
    return crc16_ccitt(record, offsetof(calibration_record_t, crc));
}

void calibration_make_defaults(calibration_record_t *record)
{
    memset(record, 0, sizeof(*record));
    record->magic = CALIBRATION_MAGIC;
    record->version = CALIBRATION_VERSION;
    record->size = sizeof(*record);
    record->gain_x_ppm = CALIBRATION_GAIN_UNITY_PPM;
    record->gain_y_ppm = CALIBRATION_GAIN_UNITY_PPM;
    record->gain_z_ppm = CALIBRATION_GAIN_UNITY_PPM;
    record->crc = calibration_crc(record);
}

calibration_status_t calibration_load(calibration_record_t *record)
{
    eeprom_read_block(record, &eeprom_calibration, sizeof(*record));

    if (record->magic != CALIBRATION_MAGIC ||
        record->version != CALIBRATION_VERSION ||
        record->size != sizeof(*record) ||
        record->crc != calibration_crc(record)) {
        /* A completely erased EEPROM is a normal first-start condition. */
        if (record->magic != 0xFFFFFFFFUL) {
            diagnostics_record_eeprom_error();
        }
        calibration_make_defaults(record);
        return CALIBRATION_STATUS_DEFAULT;
    }

    return CALIBRATION_STATUS_VALID;
}

uint8_t calibration_save(const calibration_record_t *record)
{
    calibration_record_t copy = *record;
    copy.magic = CALIBRATION_MAGIC;
    copy.version = CALIBRATION_VERSION;
    copy.size = sizeof(copy);
    copy.crc = calibration_crc(&copy);

    eeprom_update_block(&copy, &eeprom_calibration, sizeof(copy));

    calibration_record_t verify;
    eeprom_read_block(&verify, &eeprom_calibration, sizeof(verify));

    if (memcmp(&copy, &verify, sizeof(copy)) != 0U) {
        diagnostics_record_eeprom_error();
        return 0U;
    }
    return 1U;
}

void calibration_factory_reset(void)
{
    calibration_record_t erased;
    memset(&erased, 0xFF, sizeof(erased));
    eeprom_update_block(&erased, &eeprom_calibration, sizeof(erased));
}

static int32_t apply_axis(int32_t value_x100,
                          int32_t offset_x100,
                          int32_t gain_ppm)
{
    int32_t corrected = value_x100 - offset_x100;
    int64_t scaled = (int64_t)corrected * (int64_t)gain_ppm;

    if (scaled >= 0) {
        scaled += CALIBRATION_GAIN_UNITY_PPM / 2L;
    } else {
        scaled -= CALIBRATION_GAIN_UNITY_PPM / 2L;
    }

    return (int32_t)(scaled / CALIBRATION_GAIN_UNITY_PPM);
}

void calibration_apply(const calibration_record_t *record,
                       tmag5273_measurement_t *measurement)
{
    measurement->bx_x100 = apply_axis(measurement->bx_x100,
                                      record->offset_x_x100,
                                      record->gain_x_ppm);
    measurement->by_x100 = apply_axis(measurement->by_x100,
                                      record->offset_y_x100,
                                      record->gain_y_ppm);
    measurement->bz_x100 = apply_axis(measurement->bz_x100,
                                      record->offset_z_x100,
                                      record->gain_z_ppm);
}

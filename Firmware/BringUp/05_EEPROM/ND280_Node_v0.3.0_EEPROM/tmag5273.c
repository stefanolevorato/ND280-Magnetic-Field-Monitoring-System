#include "config.h"
#include <util/delay.h>
#include <stdint.h>
#include "twi1.h"
#include "tmag5273.h"

#define DEVICE_CONFIG_1  0x00U
#define DEVICE_CONFIG_2  0x01U
#define SENSOR_CONFIG_1  0x02U
#define SENSOR_CONFIG_2  0x03U
#define T_CONFIG          0x07U
#define DEVICE_ID         0x0DU
#define T_MSB_RESULT      0x10U

#define TSENSET0_X100 2500L
#define TADCT0        17508L
#define TADCRES_X10   601L

static int16_t make_i16(uint8_t msb, uint8_t lsb)
{
    return (int16_t)(((uint16_t)msb << 8) | (uint16_t)lsb);
}

uint8_t tmag5273_read_device_id(uint8_t *device_id)
{
    return twi1_read_register(TMAG_I2C_ADDRESS, DEVICE_ID, device_id);
}

uint8_t tmag5273_init(void)
{
    /* Preserve the exact known-good configuration from firmware v0.1.0. */
    if (!twi1_write_register(TMAG_I2C_ADDRESS, DEVICE_CONFIG_1, 0x00U)) return 0;
    if (!twi1_write_register(TMAG_I2C_ADDRESS, DEVICE_CONFIG_2, 0x00U)) return 0;
    if (!twi1_write_register(TMAG_I2C_ADDRESS, SENSOR_CONFIG_1, 0x74U)) return 0;
    if (!twi1_write_register(TMAG_I2C_ADDRESS, SENSOR_CONFIG_2, 0x03U)) return 0;
    if (!twi1_write_register(TMAG_I2C_ADDRESS, T_CONFIG, 0x01U)) return 0;
    return 1;
}

uint8_t tmag5273_read_raw(tmag5273_raw_sample_t *sample)
{
    uint8_t buffer[9];

    /* Trigger a conversion while the device is in standby mode. */
    if (!twi1_write_register(TMAG_I2C_ADDRESS, (uint8_t)(DEVICE_ID | 0x80U), 0x00U)) return 0;
    _delay_ms(5);

    if (!twi1_read_block(TMAG_I2C_ADDRESS, T_MSB_RESULT, buffer, 9U)) return 0;

    sample->temperature_raw = make_i16(buffer[0], buffer[1]);
    sample->bx_raw = make_i16(buffer[2], buffer[3]);
    sample->by_raw = make_i16(buffer[4], buffer[5]);
    sample->bz_raw = make_i16(buffer[6], buffer[7]);
    sample->status = buffer[8];

    return 1;
}

void tmag5273_convert_averaged_raw(int32_t temperature_raw,
                                  int32_t bx_raw,
                                  int32_t by_raw,
                                  int32_t bz_raw,
                                  uint8_t status,
                                  tmag5273_measurement_t *measurement)
{
    measurement->temperature_x100 = TSENSET0_X100
                                  + ((temperature_raw - TADCT0) * 1000L) / TADCRES_X10;

    /* TMAG5273A2 in 2x range: +/-266 mT full scale.
       Outputs are expressed in hundredths of mT. */
    measurement->bx_x100 = (bx_raw * TMAG_FULL_SCALE_X100) / 32768L;
    measurement->by_x100 = (by_raw * TMAG_FULL_SCALE_X100) / 32768L;
    measurement->bz_x100 = (bz_raw * TMAG_FULL_SCALE_X100) / 32768L;
    measurement->status = status;
}

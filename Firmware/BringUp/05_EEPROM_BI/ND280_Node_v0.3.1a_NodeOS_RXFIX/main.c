#include "config.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "uart0.h"
#include "twi1.h"
#include "board_id.h"
#include "tmag5273.h"
#include "calibration.h"
#include "commands.h"

static calibration_record_t calibration;
static calibration_status_t calibration_status;

static const char *calibration_status_text(void)
{
    if (calibration_status == CALIBRATION_STATUS_VALID) return "VALID";
    if (calibration_status == CALIBRATION_STATUS_DIRTY) return "DIRTY";
    return "DEFAULT";
}

static void fatal_error(const char *message)
{
    uart0_print("ERROR,");
    uart0_print(message);
    uart0_print("\r\n");

    while (1) {
        commands_process_pending(&calibration, &calibration_status, board_id_read());
        PORTD ^= (1 << PD7);
        _delay_ms(200);
    }
}

static uint8_t acquire_average(tmag5273_measurement_t *measurement,
                               uint8_t *valid_samples)
{
    int32_t sum_temperature = 0;
    int32_t sum_bx = 0;
    int32_t sum_by = 0;
    int32_t sum_bz = 0;
    uint8_t last_status = 0;
    uint8_t valid = 0;

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

        commands_process_pending(&calibration, &calibration_status, board_id_read());
        _delay_ms(SAMPLE_INTERVAL_MS);
    }

    *valid_samples = valid;
    if (valid == 0U) return 0;

    tmag5273_convert_averaged_raw(sum_temperature / valid,
                                  sum_bx / valid,
                                  sum_by / valid,
                                  sum_bz / valid,
                                  last_status,
                                  measurement);

    calibration_apply(&calibration, measurement);
    return 1;
}

static void print_packet(uint8_t node_id,
                         uint32_t sequence,
                         uint8_t valid_samples,
                         const tmag5273_measurement_t *measurement)
{
    uart0_print("$ND280,VER=");
    uart0_print(FIRMWARE_VERSION);
    uart0_print(",ID=");
    uart0_print_uint16(node_id);
    uart0_print(",SEQ=");
    uart0_print_uint32(sequence);
    uart0_print(",AVG=");
    uart0_print_uint16(valid_samples);
    uart0_print(",CAL=");
    uart0_print(calibration_status_text());
    uart0_print(",T=");
    uart0_print_fixed_2(measurement->temperature_x100);
    uart0_print(",BX=");
    uart0_print_fixed_2(measurement->bx_x100);
    uart0_print(",BY=");
    uart0_print_fixed_2(measurement->by_x100);
    uart0_print(",BZ=");
    uart0_print_fixed_2(measurement->bz_x100);
    uart0_print(",STATUS=0x");
    uart0_print_hex8(measurement->status);
    uart0_print("\r\n");
}

static void print_calibration_summary(void)
{
    uart0_print("CALIBRATION=");
    uart0_print(calibration_status_text());
    uart0_print(",VERSION=");
    uart0_print_uint16(calibration.version);
    uart0_print(",COUNTER=");
    uart0_print_uint32(calibration.calibration_counter);
    uart0_print("\r\n");

    uart0_print("CAL_OFFSET_X=");
    uart0_print_fixed_2(calibration.offset_x_x100);
    uart0_print(",CAL_OFFSET_Y=");
    uart0_print_fixed_2(calibration.offset_y_x100);
    uart0_print(",CAL_OFFSET_Z=");
    uart0_print_fixed_2(calibration.offset_z_x100);
    uart0_print("\r\n");
}

int main(void)
{
    uart0_init();
    twi1_init();
    board_id_init();

    DDRD |= (1 << PD5) | (1 << PD6) | (1 << PD7);
    sei();

    calibration_status = calibration_load(&calibration);

    uart0_print("\r\nND280 Magnetic Field Node v");
    uart0_print(FIRMWARE_VERSION);
    uart0_print("\r\nUART0=4800,RX_COMMANDS=ON,TWI1=PE0/PE1\r\n");
    uart0_print("AVERAGE_SAMPLES=");
    uart0_print_uint16(AVERAGE_SAMPLES);
    uart0_print(",SAMPLE_INTERVAL_MS=");
    uart0_print_uint16(SAMPLE_INTERVAL_MS);
    uart0_print("\r\nTMAG_VARIANT=");
    uart0_print(TMAG_VARIANT);
    uart0_print(",TMAG_RANGE_MT=");
    uart0_print_uint16(TMAG_FULL_SCALE_MT);
    uart0_print("\r\n");

    print_calibration_summary();

    uint8_t device_id = 0;
    if (!tmag5273_read_device_id(&device_id)) fatal_error("TMAG_DEVICE_ID_READ");

    uart0_print("TMAG_DEVICE_ID=0x");
    uart0_print_hex8(device_id);
    uart0_print("\r\n");

    if (!tmag5273_init()) fatal_error("TMAG_INIT");

    uart0_print("TMAG_INIT=OK\r\nBOARD_ID=");
    uart0_print_uint16(board_id_read());
    uart0_print("\r\nTYPE HELP FOR COMMANDS\r\n");

    uint32_t sequence = 0UL;

    while (1) {
        tmag5273_measurement_t measurement;
        uint8_t valid_samples = 0U;

        commands_process_pending(&calibration, &calibration_status, board_id_read());

        if (!acquire_average(&measurement, &valid_samples)) {
            uart0_print("ERROR,TMAG_NO_VALID_SAMPLES,SEQ=");
            uart0_print_uint32(sequence);
            uart0_print("\r\n");
            PORTD ^= (1 << PD7);
            continue;
        }

        print_packet(board_id_read(), sequence, valid_samples, &measurement);
        sequence++;
        PORTD ^= (1 << PD5);
    }
}

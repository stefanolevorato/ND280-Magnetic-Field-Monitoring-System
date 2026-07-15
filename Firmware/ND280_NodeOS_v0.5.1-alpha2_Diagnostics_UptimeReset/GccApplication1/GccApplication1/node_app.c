#include "config.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "node_app.h"
#include "board_id.h"
#include "calibration.h"
#include "commands.h"
#include "diagnostics.h"
#include "measurement_service.h"
#include "node_identity.h"
#include "status_leds.h"
#include "tmag5273.h"
#include "transport_service.h"
#include "uart0.h"
#include "twi1.h"

static calibration_record_t calibration;
static calibration_status_t calibration_status;
static uint32_t sequence;
static uint8_t node_id;

static void process_commands(void)
{
    commands_process_pending(&calibration, &calibration_status, node_id);
    status_leds_set_calibration_state(calibration_status);
    status_leds_process();
}

static void fatal_error(const char *message)
{
    transport_service_publish_error(message);
    status_leds_set_sensor_state(STATUS_SENSOR_ERROR);

    while (1) {
        process_commands();
        _delay_ms(200);
    }
}

void node_app_init(void)
{
    uint8_t device_id = 0;

    diagnostics_init();
    transport_service_init();
    twi1_init();
    board_id_init();
    status_leds_init();
    status_leds_run_post();
    sei();

    node_id = board_id_read();
    sequence = 0UL;
    calibration_status = calibration_load(&calibration);
    status_leds_set_calibration_state(calibration_status);

    if (!tmag5273_read_device_id(&device_id)) {
        fatal_error("TMAG_DEVICE_ID_READ");
    }

    if (!tmag5273_init()) {
        fatal_error("TMAG_INIT");
    }

    status_leds_set_sensor_state(STATUS_SENSOR_OK);

    transport_service_print_startup(&calibration,
                                    calibration_status,
                                    node_id,
                                    device_id);
    node_identity_print_boot_banner(node_id, calibration_status);
}

void node_app_process(void)
{
    measurement_result_t result;

    process_commands();

    if (!measurement_service_acquire_average(&calibration,
                                             &result,
                                             process_commands)) {
        uart0_print("ERROR,TMAG_NO_VALID_SAMPLES,SEQ=");
        uart0_print_uint32(sequence);
        uart0_print("\r\n");
        status_leds_set_sensor_state(STATUS_SENSOR_ERROR);
        diagnostics_record_measurement_failed();
        return;
    }

    status_leds_set_sensor_state(STATUS_SENSOR_OK);
    diagnostics_record_measurement_ok();

    transport_service_publish_measurement(node_id,
                                          sequence,
                                          calibration_status,
                                          &result);
    sequence++;

    /* PD5 remains the established heartbeat: one toggle per published packet. */
    PORTD ^= (1 << PD5);
}

#include "config.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "node_app.h"
#include "board_id.h"
#include "calibration.h"
#include "commands.h"
#include "measurement_service.h"
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
}

static void fatal_error(const char *message)
{
    transport_service_publish_error(message);

    while (1) {
        process_commands();
        PORTD ^= (1 << PD7);
        _delay_ms(200);
    }
}

void node_app_init(void)
{
    uint8_t device_id = 0;

    transport_service_init();
    twi1_init();
    board_id_init();

    DDRD |= (1 << PD5) | (1 << PD6) | (1 << PD7);
    sei();

    node_id = board_id_read();
    sequence = 0UL;
    calibration_status = calibration_load(&calibration);

    if (!tmag5273_read_device_id(&device_id)) {
        fatal_error("TMAG_DEVICE_ID_READ");
    }

    if (!tmag5273_init()) {
        fatal_error("TMAG_INIT");
    }

    transport_service_print_startup(&calibration,
                                    calibration_status,
                                    node_id,
                                    device_id);
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
        PORTD ^= (1 << PD7);
        return;
    }

    transport_service_publish_measurement(node_id,
                                          sequence,
                                          calibration_status,
                                          &result);
    sequence++;
    PORTD ^= (1 << PD5);
}

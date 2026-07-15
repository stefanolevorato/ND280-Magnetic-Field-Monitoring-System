#include "config.h"
#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#include "status_leds.h"

/*
 * Stable LED meanings for the current single-sensor node:
 *
 * PD5 HEARTBEAT - controlled by node_app after each published measurement.
 * PD6 SENSOR    - ON: sensor operational; blinking: sensor/read error;
 *                 OFF: startup/not initialized.
 * PD7 SYSTEM    - OFF: CAL=DEFAULT; ON: CAL=VALID;
 *                 slow blink: CAL=DIRTY.
 *
 * The power-on self-test only verifies the three LED circuits. After POST,
 * the LEDs return to their normal runtime meanings.
 */

#define HEARTBEAT_LED PD5
#define SENSOR_LED    PD6
#define SYSTEM_LED    PD7
#define ALL_STATUS_LEDS ((1 << HEARTBEAT_LED) | (1 << SENSOR_LED) | (1 << SYSTEM_LED))

#define SENSOR_ERROR_TOGGLE_TICKS 2U
#define CAL_DIRTY_TOGGLE_TICKS    5U

#define POST_SINGLE_LED_MS 120
#define POST_ALL_LEDS_MS   250
#define POST_GAP_MS         60

static status_sensor_state_t sensor_state = STATUS_SENSOR_STARTING;
static calibration_status_t calibration_state = CALIBRATION_STATUS_DEFAULT;
static uint8_t sensor_ticks = 0U;
static uint8_t calibration_ticks = 0U;

static void leds_all_off(void)
{
    PORTD &= ~ALL_STATUS_LEDS;
}

void status_leds_init(void)
{
    DDRD |= ALL_STATUS_LEDS;
    leds_all_off();

    sensor_state = STATUS_SENSOR_STARTING;
    calibration_state = CALIBRATION_STATUS_DEFAULT;
    sensor_ticks = 0U;
    calibration_ticks = 0U;
}

void status_leds_run_post(void)
{
    /* PD5 -> PD6 -> PD7 -> all LEDs. */
    leds_all_off();

    PORTD |= (1 << HEARTBEAT_LED);
    _delay_ms(POST_SINGLE_LED_MS);
    leds_all_off();
    _delay_ms(POST_GAP_MS);

    PORTD |= (1 << SENSOR_LED);
    _delay_ms(POST_SINGLE_LED_MS);
    leds_all_off();
    _delay_ms(POST_GAP_MS);

    PORTD |= (1 << SYSTEM_LED);
    _delay_ms(POST_SINGLE_LED_MS);
    leds_all_off();
    _delay_ms(POST_GAP_MS);

    PORTD |= ALL_STATUS_LEDS;
    _delay_ms(POST_ALL_LEDS_MS);
    leds_all_off();

    /* Restore the startup state used by the runtime state machine. */
    sensor_state = STATUS_SENSOR_STARTING;
    calibration_state = CALIBRATION_STATUS_DEFAULT;
    sensor_ticks = 0U;
    calibration_ticks = 0U;
}

void status_leds_set_sensor_state(status_sensor_state_t state)
{
    if (sensor_state != state) {
        sensor_state = state;
        sensor_ticks = 0U;

        if (state == STATUS_SENSOR_STARTING) {
            PORTD &= ~(1 << SENSOR_LED);
        } else if (state == STATUS_SENSOR_OK) {
            PORTD |= (1 << SENSOR_LED);
        } else {
            PORTD &= ~(1 << SENSOR_LED);
        }
    }
}

void status_leds_set_calibration_state(calibration_status_t state)
{
    if (calibration_state != state) {
        calibration_state = state;
        calibration_ticks = 0U;

        if (state == CALIBRATION_STATUS_VALID) {
            PORTD |= (1 << SYSTEM_LED);
        } else {
            PORTD &= ~(1 << SYSTEM_LED);
        }
    }
}

void status_leds_process(void)
{
    if (sensor_state == STATUS_SENSOR_OK) {
        PORTD |= (1 << SENSOR_LED);
    } else if (sensor_state == STATUS_SENSOR_STARTING) {
        PORTD &= ~(1 << SENSOR_LED);
    } else {
        sensor_ticks++;
        if (sensor_ticks >= SENSOR_ERROR_TOGGLE_TICKS) {
            sensor_ticks = 0U;
            PORTD ^= (1 << SENSOR_LED);
        }
    }

    if (calibration_state == CALIBRATION_STATUS_DEFAULT) {
        PORTD &= ~(1 << SYSTEM_LED);
    } else if (calibration_state == CALIBRATION_STATUS_VALID) {
        PORTD |= (1 << SYSTEM_LED);
    } else {
        calibration_ticks++;
        if (calibration_ticks >= CAL_DIRTY_TOGGLE_TICKS) {
            calibration_ticks = 0U;
            PORTD ^= (1 << SYSTEM_LED);
        }
    }
}

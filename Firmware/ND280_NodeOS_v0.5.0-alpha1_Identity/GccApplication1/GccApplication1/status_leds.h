#ifndef STATUS_LEDS_H
#define STATUS_LEDS_H

#include "calibration.h"

typedef enum {
    STATUS_SENSOR_STARTING = 0,
    STATUS_SENSOR_OK,
    STATUS_SENSOR_ERROR
} status_sensor_state_t;

void status_leds_init(void);
void status_leds_run_post(void);
void status_leds_set_sensor_state(status_sensor_state_t state);
void status_leds_set_calibration_state(calibration_status_t state);
void status_leds_process(void);

#endif

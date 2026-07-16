#ifndef TRANSPORT_SERVICE_H
#define TRANSPORT_SERVICE_H

#include <stdint.h>
#include "calibration.h"
#include "measurement_service.h"

void transport_service_init(void);
void transport_service_print_startup(const calibration_record_t *calibration,
                                     calibration_status_t status,
                                     uint8_t node_id,
                                     uint8_t device_id);
void transport_service_publish_measurement(uint8_t node_id,
                                           uint32_t sequence,
                                           calibration_status_t status,
                                           const measurement_result_t *result);
void transport_service_publish_error(const char *message);
const char *transport_service_calibration_status_text(calibration_status_t status);

#endif

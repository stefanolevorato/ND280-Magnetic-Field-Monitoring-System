#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>
#include "calibration.h"

void commands_process_pending(calibration_record_t *calibration,
                              calibration_status_t *status,
                              uint8_t node_id);

#endif

#ifndef NODE_IDENTITY_H
#define NODE_IDENTITY_H

#include <stdint.h>
#include "calibration.h"

void node_identity_print_info(uint8_t node_id,
                              calibration_status_t calibration_status);
void node_identity_print_boot_banner(uint8_t node_id,
                                     calibration_status_t calibration_status);

#endif

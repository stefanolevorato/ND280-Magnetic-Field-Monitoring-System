#include "config.h"
#include "node_identity.h"
#include "uart0.h"

static const char *calibration_status_text(calibration_status_t status)
{
    if (status == CALIBRATION_STATUS_VALID) return "VALID";
    if (status == CALIBRATION_STATUS_DIRTY) return "DIRTY";
    return "DEFAULT";
}

void node_identity_print_info(uint8_t node_id,
                              calibration_status_t calibration_status)
{
    uart0_print("$ND280INFO,ID=");
    uart0_print_uint16(node_id);
    uart0_print(",NODEOS=");
    uart0_print(NODEOS_VERSION);
    uart0_print(",PROTO=");
    uart0_print_uint16(NODEOS_PROTOCOL_VERSION);
    uart0_print(",BOARD_REV=");
    uart0_print(NODE_BOARD_REVISION);
    uart0_print(",HW=");
    uart0_print(NODE_HARDWARE_NAME);
    uart0_print(",MCU=");
    uart0_print(NODE_MCU_NAME);
    uart0_print(",SENSOR=");
    uart0_print(NODE_SENSOR_NAME);
    uart0_print(",RANGE=");
    uart0_print_uint16(TMAG_FULL_SCALE_MT);
    uart0_print(",AVG=");
    uart0_print_uint16(AVERAGE_SAMPLES);
    uart0_print(",EEPROM_VER=");
    uart0_print_uint16(CALIBRATION_VERSION);
    uart0_print(",CAL=");
    uart0_print(calibration_status_text(calibration_status));
    uart0_print(",BUILD_DATE=");
    uart0_print(__DATE__);
    uart0_print(",BUILD_TIME=");
    uart0_print(__TIME__);
    uart0_print("\r\n");
}

void node_identity_print_boot_banner(uint8_t node_id,
                                     calibration_status_t calibration_status)
{
    uart0_print("NODEOS_VERSION=");
    uart0_print(NODEOS_VERSION);
    uart0_print(",PROTOCOL=");
    uart0_print_uint16(NODEOS_PROTOCOL_VERSION);
    uart0_print(",BOARD_REV=");
    uart0_print(NODE_BOARD_REVISION);
    uart0_print(",MCU=");
    uart0_print(NODE_MCU_NAME);
    uart0_print(",SENSOR=");
    uart0_print(NODE_SENSOR_NAME);
    uart0_print(",NODE_ID=");
    uart0_print_uint16(node_id);
    uart0_print(",CAL=");
    uart0_print(calibration_status_text(calibration_status));
    uart0_print("\r\nBUILD_DATE=");
    uart0_print(__DATE__);
    uart0_print(",BUILD_TIME=");
    uart0_print(__TIME__);
    uart0_print("\r\n");
}

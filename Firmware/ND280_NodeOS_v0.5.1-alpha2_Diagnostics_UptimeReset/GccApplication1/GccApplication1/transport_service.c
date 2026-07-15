#include "config.h"
#include <stdint.h>

#include "transport_service.h"
#include "uart0.h"
#include "diagnostics.h"

const char *transport_service_calibration_status_text(calibration_status_t status)
{
    if (status == CALIBRATION_STATUS_VALID) return "VALID";
    if (status == CALIBRATION_STATUS_DIRTY) return "DIRTY";
    return "DEFAULT";
}

void transport_service_init(void)
{
    uart0_init();
}

void transport_service_publish_error(const char *message)
{
    uart0_print("ERROR,");
    uart0_print(message);
    uart0_print("\r\n");
}

static void print_calibration_summary(const calibration_record_t *calibration,
                                      calibration_status_t status)
{
    uart0_print("CALIBRATION=");
    uart0_print(transport_service_calibration_status_text(status));
    uart0_print(",VERSION=");
    uart0_print_uint16(calibration->version);
    uart0_print(",COUNTER=");
    uart0_print_uint32(calibration->calibration_counter);
    uart0_print("\r\n");

    uart0_print("CAL_OFFSET_X=");
    uart0_print_fixed_2(calibration->offset_x_x100);
    uart0_print(",CAL_OFFSET_Y=");
    uart0_print_fixed_2(calibration->offset_y_x100);
    uart0_print(",CAL_OFFSET_Z=");
    uart0_print_fixed_2(calibration->offset_z_x100);
    uart0_print("\r\n");
}

void transport_service_print_startup(const calibration_record_t *calibration,
                                     calibration_status_t status,
                                     uint8_t node_id,
                                     uint8_t device_id)
{
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

    print_calibration_summary(calibration, status);

    uart0_print("TMAG_DEVICE_ID=0x");
    uart0_print_hex8(device_id);
    uart0_print("\r\nTMAG_INIT=OK\r\nBOARD_ID=");
    uart0_print_uint16(node_id);
    uart0_print("\r\nRESET_CAUSE=");
    uart0_print(diagnostics_get_reset_cause_text());
    uart0_print(",BOOT_COUNT=");
    uart0_print_uint32(diagnostics_get_boot_count());
    uart0_print("\r\nTYPE HELP FOR COMMANDS\r\n");
}

void transport_service_publish_measurement(uint8_t node_id,
                                           uint32_t sequence,
                                           calibration_status_t status,
                                           const measurement_result_t *result)
{
    uart0_print("$ND280,VER=");
    uart0_print(FIRMWARE_VERSION);
    uart0_print(",ID=");
    uart0_print_uint16(node_id);
    uart0_print(",SEQ=");
    uart0_print_uint32(sequence);
    uart0_print(",AVG=");
    uart0_print_uint16(result->valid_samples);
    uart0_print(",CAL=");
    uart0_print(transport_service_calibration_status_text(status));
    uart0_print(",T=");
    uart0_print_fixed_2(result->value.temperature_x100);
    uart0_print(",BX=");
    uart0_print_fixed_2(result->value.bx_x100);
    uart0_print(",BY=");
    uart0_print_fixed_2(result->value.by_x100);
    uart0_print(",BZ=");
    uart0_print_fixed_2(result->value.bz_x100);
    uart0_print(",STATUS=0x");
    uart0_print_hex8(result->value.status);
    uart0_print("\r\n");
}

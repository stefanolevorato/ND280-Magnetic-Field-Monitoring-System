#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "config.h"
#include "uart0.h"
#include "node_identity.h"
#include "diagnostics.h"
#include "watchdog_service.h"

#define COMMAND_DUPLICATE_GUARD_MS 1000UL

static char last_command[UART0_COMMAND_MAX_LENGTH];
static uint32_t last_command_seen_ms = 0UL;
static uint8_t last_command_valid = 0U;

static uint8_t command_is_duplicate(const char *line)
{
    uint32_t now_ms = diagnostics_get_uptime_milliseconds();

    if (last_command_valid && strcmp(line, last_command) == 0) {
        uint32_t elapsed = now_ms - last_command_seen_ms;
        last_command_seen_ms = now_ms;
        if (elapsed < COMMAND_DUPLICATE_GUARD_MS) {
            diagnostics_record_uart_duplicate_command();
            return 1U;
        }
    }

    strncpy(last_command, line, sizeof(last_command) - 1U);
    last_command[sizeof(last_command) - 1U] = '\0';
    last_command_seen_ms = now_ms;
    last_command_valid = 1U;
    return 0U;
}

static const char *status_text(calibration_status_t status)
{
    if (status == CALIBRATION_STATUS_VALID) return "VALID";
    if (status == CALIBRATION_STATUS_DIRTY) return "DIRTY";
    return "DEFAULT";
}

static void response_ok(const char *command)
{
    uart0_print("$ND280RSP,OK,CMD=");
    uart0_print(command);
    uart0_print("\r\n");
}

static void response_error(const char *reason)
{
    uart0_print("$ND280RSP,ERROR,REASON=");
    uart0_print(reason);
    uart0_print("\r\n");
}

static uint8_t parse_i32(const char *text, int32_t *value)
{
    char *end = 0;
    long parsed = strtol(text, &end, 10);
    if (text == end || *end != '\0') return 0U;
    *value = (int32_t)parsed;
    return 1U;
}

static uint8_t parse_u32(const char *text, uint32_t *value)
{
    char *end = 0;
    unsigned long parsed = strtoul(text, &end, 10);
    if (text == end || *end != '\0') return 0U;
    *value = (uint32_t)parsed;
    return 1U;
}

static void print_calibration(const calibration_record_t *cal,
                              calibration_status_t status)
{
    uart0_print("$ND280CAL,STATE=");
    uart0_print(status_text(status));
    uart0_print(",VER=");
    uart0_print_uint16(cal->version);
    uart0_print(",COUNT=");
    uart0_print_uint32(cal->calibration_counter);
    uart0_print(",OX=");
    uart0_print_int32(cal->offset_x_x100);
    uart0_print(",OY=");
    uart0_print_int32(cal->offset_y_x100);
    uart0_print(",OZ=");
    uart0_print_int32(cal->offset_z_x100);
    uart0_print(",GX=");
    uart0_print_int32(cal->gain_x_ppm);
    uart0_print(",GY=");
    uart0_print_int32(cal->gain_y_ppm);
    uart0_print(",GZ=");
    uart0_print_int32(cal->gain_z_ppm);
    uart0_print(",NX=");
    uart0_print_uint32(cal->noise_sigma_x_x10000);
    uart0_print(",NY=");
    uart0_print_uint32(cal->noise_sigma_y_x10000);
    uart0_print(",NZ=");
    uart0_print_uint32(cal->noise_sigma_z_x10000);
    uart0_print(",TCAL=");
    uart0_print_int32(cal->calibration_temperature_x100);
    uart0_print("\r\n");
}

static uint8_t get_three_i32(char *a, char *b, char *c,
                             int32_t *va, int32_t *vb, int32_t *vc)
{
    return a && b && c &&
           parse_i32(a, va) && parse_i32(b, vb) && parse_i32(c, vc);
}

static uint8_t get_three_u32(char *a, char *b, char *c,
                             uint32_t *va, uint32_t *vb, uint32_t *vc)
{
    return a && b && c &&
           parse_u32(a, va) && parse_u32(b, vb) && parse_u32(c, vc);
}

static void process_command(char *line,
                            calibration_record_t *cal,
                            calibration_status_t *status,
                            uint8_t node_id)
{
    char *command = strtok(line, " ");
    if (!command) return;

    if (strcmp(command, "HELP") == 0) {
        uart0_print("$ND280RSP,COMMANDS=HELP|INFO|IDENTITY|DIAG|DIAG WDT TEST|CAL READ|CAL OFFSET x y z|CAL GAIN x y z|CAL NOISE x y z|CAL TEMP t|CAL SAVE|CAL RESET\r\n");
        return;
    }

    if (strcmp(command, "INFO") == 0 || strcmp(command, "IDENTITY") == 0) {
        node_identity_print_info(node_id, *status);
        return;
    }

    if (strcmp(command, "DIAG") == 0) {
        char *diag_subcommand = strtok(0, " ");
        if (diag_subcommand == 0) {
            diagnostics_print_response(node_id);
            return;
        }

        if (strcmp(diag_subcommand, "WDT") == 0) {
            char *action = strtok(0, " ");
            if (action != 0 && strcmp(action, "TEST") == 0) {
                watchdog_service_force_test_reset();
                return;
            }
            response_error("BAD_DIAG_WDT_COMMAND");
            return;
        }

        response_error("UNKNOWN_DIAG_SUBCOMMAND");
        return;
    }

    if (strcmp(command, "CAL") != 0) {
        response_error("UNKNOWN_COMMAND");
        return;
    }

    char *subcommand = strtok(0, " ");
    if (!subcommand) {
        response_error("MISSING_CAL_SUBCOMMAND");
        return;
    }

    if (strcmp(subcommand, "READ") == 0) {
        print_calibration(cal, *status);
        return;
    }

    if (strcmp(subcommand, "OFFSET") == 0) {
        int32_t x, y, z;
        if (!get_three_i32(strtok(0, " "), strtok(0, " "), strtok(0, " "), &x, &y, &z)) {
            response_error("BAD_OFFSET_ARGUMENTS");
            return;
        }
        cal->offset_x_x100 = x;
        cal->offset_y_x100 = y;
        cal->offset_z_x100 = z;
        *status = CALIBRATION_STATUS_DIRTY;
        response_ok("CAL_OFFSET");
        return;
    }

    if (strcmp(subcommand, "GAIN") == 0) {
        int32_t x, y, z;
        if (!get_three_i32(strtok(0, " "), strtok(0, " "), strtok(0, " "), &x, &y, &z) ||
            x <= 0L || y <= 0L || z <= 0L) {
            response_error("BAD_GAIN_ARGUMENTS");
            return;
        }
        cal->gain_x_ppm = x;
        cal->gain_y_ppm = y;
        cal->gain_z_ppm = z;
        *status = CALIBRATION_STATUS_DIRTY;
        response_ok("CAL_GAIN");
        return;
    }

    if (strcmp(subcommand, "NOISE") == 0) {
        uint32_t x, y, z;
        if (!get_three_u32(strtok(0, " "), strtok(0, " "), strtok(0, " "), &x, &y, &z)) {
            response_error("BAD_NOISE_ARGUMENTS");
            return;
        }
        cal->noise_sigma_x_x10000 = x;
        cal->noise_sigma_y_x10000 = y;
        cal->noise_sigma_z_x10000 = z;
        *status = CALIBRATION_STATUS_DIRTY;
        response_ok("CAL_NOISE");
        return;
    }

    if (strcmp(subcommand, "TEMP") == 0) {
        int32_t value;
        char *argument = strtok(0, " ");
        if (!argument || !parse_i32(argument, &value)) {
            response_error("BAD_TEMP_ARGUMENT");
            return;
        }
        cal->calibration_temperature_x100 = value;
        *status = CALIBRATION_STATUS_DIRTY;
        response_ok("CAL_TEMP");
        return;
    }

    if (strcmp(subcommand, "SAVE") == 0) {
        cal->calibration_counter++;
        if (calibration_save(cal)) {
            *status = CALIBRATION_STATUS_VALID;
            response_ok("CAL_SAVE");
            print_calibration(cal, *status);
        } else {
            cal->calibration_counter--;
            response_error("EEPROM_VERIFY_FAILED");
        }
        return;
    }

    if (strcmp(subcommand, "RESET") == 0) {
        calibration_factory_reset();
        calibration_make_defaults(cal);
        *status = CALIBRATION_STATUS_DEFAULT;
        response_ok("CAL_RESET");
        print_calibration(cal, *status);
        return;
    }

    response_error("UNKNOWN_CAL_SUBCOMMAND");
}

void commands_process_pending(calibration_record_t *calibration,
                              calibration_status_t *status,
                              uint8_t node_id)
{
    char line[UART0_COMMAND_MAX_LENGTH];

    if (uart0_rx_overflowed()) {
        uart0_clear_rx_overflow();
        diagnostics_record_uart_rx_overflow();
        response_error("UART_RX_OVERFLOW");
    }

    /* Process at most one complete command per application pass.
     * This prevents a continuous RX stream from starving acquisition and
     * watchdog servicing. Remaining queued commands are handled later. */
    if (uart0_readline(line, sizeof(line))) {
        if (command_is_duplicate(line)) return;
        diagnostics_record_uart_command();
        process_command(line, calibration, status, node_id);
        watchdog_service_kick();
    }
}

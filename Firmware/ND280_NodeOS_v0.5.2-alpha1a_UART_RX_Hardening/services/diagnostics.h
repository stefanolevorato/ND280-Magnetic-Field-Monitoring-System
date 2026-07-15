#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdint.h>

typedef struct
{
    uint32_t measurements_ok;
    uint32_t measurements_failed;
    uint32_t twi_errors;
    uint32_t uart_commands;
    uint32_t uart_rx_overflows;
    uint32_t eeprom_errors;
    uint32_t watchdog_resets;
} diagnostics_counters_t;

void diagnostics_init(void);
uint32_t diagnostics_get_uptime_seconds(void);
uint32_t diagnostics_get_boot_count(void);
uint32_t diagnostics_get_watchdog_reset_count(void);
uint8_t diagnostics_get_reset_flags(void);
void diagnostics_print_reset_causes(void);

void diagnostics_record_measurement_ok(void);
void diagnostics_record_measurement_failed(void);
void diagnostics_record_twi_error(void);
void diagnostics_record_uart_command(void);
void diagnostics_record_uart_rx_overflow(void);
void diagnostics_record_eeprom_error(void);

void diagnostics_get_counters(diagnostics_counters_t *destination);
void diagnostics_print_response(uint8_t node_id);

#endif

#include "config.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>

#include "diagnostics.h"
#include "uart0.h"

static uint32_t EEMEM eeprom_boot_count;

static volatile uint32_t uptime_ms = 0UL;
static uint32_t boot_count = 0UL;
static uint8_t reset_flags = 0U;
static diagnostics_counters_t counters;

ISR(TIMER0_COMPA_vect)
{
    uptime_ms++;
}

static void timer0_init_1ms(void)
{
    TCCR0A = (1 << WGM01);                  /* CTC mode. */
    TCCR0B = (1 << CS01);                   /* Prescaler 8. */
    OCR0A = DIAGNOSTICS_TIMER0_COMPARE;     /* 1 kHz compare. */
    TCNT0 = 0U;
    TIMSK0 |= (1 << OCIE0A);
}

void diagnostics_init(void)
{
    reset_flags = MCUSR;
    MCUSR = 0U;

    counters.measurements_ok = 0UL;
    counters.measurements_failed = 0UL;
    counters.twi_errors = 0UL;
    counters.uart_commands = 0UL;
    counters.uart_rx_overflows = 0UL;
    counters.eeprom_errors = 0UL;
    uptime_ms = 0UL;

    boot_count = eeprom_read_dword(&eeprom_boot_count);
    if (boot_count == 0xFFFFFFFFUL) boot_count = 0UL;
    boot_count++;
    eeprom_update_dword(&eeprom_boot_count, boot_count);

    timer0_init_1ms();
}

uint32_t diagnostics_get_uptime_ms(void)
{
    uint8_t saved_sreg = SREG;
    cli();
    uint32_t value = uptime_ms;
    SREG = saved_sreg;
    return value;
}

uint32_t diagnostics_get_boot_count(void)
{
    return boot_count;
}

uint8_t diagnostics_get_reset_flags(void)
{
    return reset_flags;
}

const char *diagnostics_get_reset_cause_text(void)
{
#ifdef WDRF
    if (reset_flags & (1 << WDRF)) return "WATCHDOG";
#endif
#ifdef BORF
    if (reset_flags & (1 << BORF)) return "BROWN_OUT";
#endif
#ifdef EXTRF
    if (reset_flags & (1 << EXTRF)) return "EXTERNAL";
#endif
#ifdef PORF
    if (reset_flags & (1 << PORF)) return "POWER_ON";
#endif
#ifdef JTRF
    if (reset_flags & (1 << JTRF)) return "JTAG";
#endif
    return "UNKNOWN";
}

void diagnostics_record_measurement_ok(void)       { counters.measurements_ok++; }
void diagnostics_record_measurement_failed(void)   { counters.measurements_failed++; }
void diagnostics_record_twi_error(void)            { counters.twi_errors++; }
void diagnostics_record_uart_command(void)         { counters.uart_commands++; }
void diagnostics_record_uart_rx_overflow(void)     { counters.uart_rx_overflows++; }
void diagnostics_record_eeprom_error(void)         { counters.eeprom_errors++; }

void diagnostics_get_counters(diagnostics_counters_t *destination)
{
    if (destination != 0) *destination = counters;
}

void diagnostics_print_response(uint8_t node_id)
{
    uart0_print("$ND280DIAG,ID=");
    uart0_print_uint16(node_id);
    uart0_print(",UPTIME_MS=");
    uart0_print_uint32(diagnostics_get_uptime_ms());
    uart0_print(",BOOTS=");
    uart0_print_uint32(boot_count);
    uart0_print(",MEAS_OK=");
    uart0_print_uint32(counters.measurements_ok);
    uart0_print(",MEAS_FAIL=");
    uart0_print_uint32(counters.measurements_failed);
    uart0_print(",TWI_ERR=");
    uart0_print_uint32(counters.twi_errors);
    uart0_print(",UART_CMD=");
    uart0_print_uint32(counters.uart_commands);
    uart0_print(",UART_OVF=");
    uart0_print_uint32(counters.uart_rx_overflows);
    uart0_print(",EEPROM_ERR=");
    uart0_print_uint32(counters.eeprom_errors);
    uart0_print(",RESET=");
    uart0_print(diagnostics_get_reset_cause_text());
    uart0_print(",RESET_FLAGS=0x");
    uart0_print_hex8(reset_flags);
    uart0_print("\r\n");
}

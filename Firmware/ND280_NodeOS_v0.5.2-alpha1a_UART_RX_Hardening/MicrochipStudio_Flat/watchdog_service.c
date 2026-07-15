#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "watchdog_service.h"
#include "uart0.h"

void watchdog_service_init(void)
{
    /* Four seconds comfortably exceeds the normal acquisition cycle. */
    wdt_enable(WDTO_4S);
    wdt_reset();
}

void watchdog_service_kick(void)
{
    wdt_reset();
}

void watchdog_service_force_test_reset(void)
{
    uart0_print("$ND280RSP,OK,CMD=DIAG_WDT_TEST,RESET_IN_MS=1000\r\n");

    /* Use a short timeout for a deterministic controlled test. */
    cli();
    wdt_enable(WDTO_1S);
    sei();

    /* Deliberately do not service the watchdog. */
    while (1) { }
}

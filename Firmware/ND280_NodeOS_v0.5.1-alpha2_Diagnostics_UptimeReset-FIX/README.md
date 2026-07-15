# ND280 NodeOS v0.5.1-alpha2 — Diagnostics uptime/reset update

Use the **MicrochipStudio_Flat** directory for the operational Microchip Studio project.

## Changes

- Diagnostic uptime is now exported as `UPTIME_S` using a 32-bit seconds counter.
- Rollover is approximately 136 years instead of 49.7 days.
- A private 1 ms accumulator remains available for the Timer0 time base.
- `MCUSR` is captured in the AVR `.init3` startup section and immediately cleared.
- The watchdog is disabled during early startup.
- Every asserted reset flag is printed, separated by `|`.

Example:

```text
$ND280DIAG,ID=4,UPTIME_S=62,BOOTS=2,MEAS_OK=44,MEAS_FAIL=0,TWI_ERR=0,UART_CMD=3,UART_OVF=0,EEPROM_ERR=0,RESET=POWER_ON|EXTERNAL|BROWN_OUT,RESET_FLAGS=0x07
```

## Important

The first boot after ISP programming may legitimately show several reset flags. Perform dedicated reset tests after flashing:

- push-button reset: expect `EXTERNAL`;
- clean power cycle: normally expect `POWER_ON`, possibly also `BROWN_OUT` depending on supply ramp and fuse configuration;
- watchdog reset will be tested when the watchdog service is introduced.

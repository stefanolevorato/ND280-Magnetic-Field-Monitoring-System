# NodeOS v0.5.2-alpha1 - Watchdog and Persistent Diagnostics

This release adds a normally active hardware watchdog and a controlled watchdog-reset test.

Use `MicrochipStudio_Flat` for the tested Microchip Studio workflow. Add all `.c` and `.h` files, including:

- `watchdog_service.c`
- `watchdog_service.h`

## Watchdog policy

- Normal timeout: 4 seconds.
- The watchdog is serviced from the application loop and acquisition background processing.
- A blocked main loop therefore causes automatic recovery.
- Watchdog resets are counted persistently in EEPROM.

## Commands

`DIAG` now includes `WDT_RESETS`.

Controlled test command:

```text
DIAG WDT TEST
```

The node acknowledges the command, stops servicing the watchdog and resets after approximately one second. After reboot, `RESET=WATCHDOG` and `WDT_RESETS` must have increased.

# ND280 Monitor 0.6.0 - Diagnostics Dashboard

This release extends the working 0.5.0 Calibration Control Panel with a dedicated NodeOS diagnostics dashboard.

## New functions

- Parses `$ND280DIAG` packets.
- Displays uptime in days/hours/minutes/seconds.
- Displays boot count and persistent watchdog-reset count.
- Displays successful and failed measurements.
- Displays TWI, UART-overflow and EEPROM error counters.
- Displays the last reset cause and raw reset flags.
- Computes a simple `OK` / `ATTENTION` health indication.
- Automatically requests `DIAG` every 10 seconds while the dashboard is open.
- Provides a confirmed `DIAG WDT TEST` button.
- Adds a DIAG quick-command button to the main Control Panel.

## Run

Double-click `run_monitor.bat`, select the serial port at 4800 baud, connect, and open **Diagnostics...**.

## Expected firmware

NodeOS v0.5.2-alpha1 or later, producing packets such as:

```text
$ND280DIAG,ID=4,UPTIME_S=120,BOOTS=17,MEAS_OK=82,MEAS_FAIL=0,TWI_ERR=0,UART_CMD=3,UART_OVF=0,EEPROM_ERR=0,WDT_RESETS=1,RESET=WATCHDOG,RESET_FLAGS=0x08
```


## v0.6.1 diagnostic polling fix

The Diagnostics window now requests one DIAG snapshot when opened. Further updates are manual through Refresh DIAG. Automatic 10-second polling was removed because it could interact badly with some Pico/MicroPython USB-UART bridges and provoke watchdog resets.

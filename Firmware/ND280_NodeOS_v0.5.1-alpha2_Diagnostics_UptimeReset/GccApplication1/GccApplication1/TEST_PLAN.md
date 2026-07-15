# Test Plan — NodeOS v0.5.1-alpha2

## Build

1. Create/select an ATmega328PB GCC C project.
2. Import every `.c` and `.h` file from `MicrochipStudio_Flat`.
3. Clean, rebuild and flash.
4. Confirm startup banner reports `0.5.1-alpha2`.

## Uptime

1. Send `DIAG` shortly after boot.
2. Confirm the field name is `UPTIME_S`.
3. Wait at least 10 seconds and send `DIAG` again.
4. Confirm uptime increased by approximately the elapsed number of seconds.

## Reset flags

### External reset

1. Press the board reset button.
2. Send `DIAG`.
3. Confirm `RESET` includes `EXTERNAL` and `RESET_FLAGS` includes the EXTRF bit.

### Power cycle

1. Remove power completely.
2. Wait several seconds.
3. Restore power.
4. Send `DIAG`.
5. Confirm `RESET` includes `POWER_ON`; `BROWN_OUT` may also be present depending on the supply ramp and fuse state.

### Multiple flags

If multiple bits are asserted, confirm every cause is printed with `|`, for example:

```text
RESET=POWER_ON|EXTERNAL|BROWN_OUT
```

## Regression

- Measurements continue normally.
- `MEAS_OK` increments.
- `MEAS_FAIL`, `TWI_ERR`, `UART_OVF`, and `EEPROM_ERR` remain zero in normal operation.
- `HELP`, `INFO`, `CAL READ`, and `DIAG` respond correctly.
- Status LEDs retain their validated behaviour.

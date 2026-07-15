# Test Plan — NodeOS v0.5.1-alpha1 Diagnostics

## D-001 Build and boot

1. Import every `.c` and `.h` file from `MicrochipStudio_Flat`.
2. Confirm target device is ATmega328PB.
3. Clean, rebuild and flash.
4. Confirm the LED POST and normal measurement stream.
5. Confirm banner version `0.5.1-alpha1`.

## D-002 DIAG command

Send `DIAG` and verify one `$ND280DIAG` line is returned.
Expected on a healthy node:

- `UPTIME_MS` increases;
- `MEAS_OK` increases;
- `MEAS_FAIL=0`;
- `TWI_ERR=0`;
- `UART_CMD` increases after each command;
- `UART_OVF=0`;
- `EEPROM_ERR=0` for a valid or erased EEPROM.

## D-003 Persistent boot count

1. Record `BOOTS`.
2. Press RESET.
3. Send `DIAG` again.
4. Confirm `BOOTS` increased by one and `RESET=EXTERNAL`.
5. Power-cycle and confirm another increment with `RESET=POWER_ON`.

## D-004 Calibration regression

Verify `CAL READ`, `CAL OFFSET`, `CAL SAVE`, power-cycle persistence and `CAL RESET` still work.

## D-005 Overnight test

Run for at least 8 hours and save:

- initial and final `DIAG` responses;
- monitor CSV;
- packet-loss count;
- firmware and monitor versions.

Pass criteria: no resets, no failed measurements, no TWI/EEPROM/UART overflow errors.

# ND280 NodeOS v0.5.1-alpha1 — Diagnostics

This release adds internal reliability counters without changing the sensor acquisition protocol.

## Operational folder

Use `MicrochipStudio_Flat` in Microchip Studio, as agreed for the working flow.
Add the two new files:

- `diagnostics.c`
- `diagnostics.h`

## New command

```text
DIAG
```

Expected response:

```text
$ND280DIAG,ID=4,UPTIME_MS=...,BOOTS=...,MEAS_OK=...,MEAS_FAIL=0,TWI_ERR=0,UART_CMD=...,UART_OVF=0,EEPROM_ERR=0,RESET=POWER_ON,RESET_FLAGS=0x01
```

## Counters

- `UPTIME_MS`: milliseconds since the latest reset.
- `BOOTS`: persistent EEPROM boot counter.
- `MEAS_OK`: complete averaged measurements successfully published.
- `MEAS_FAIL`: averaged acquisitions with no valid samples.
- `TWI_ERR`: failed TMAG/TWI transactions.
- `UART_CMD`: complete commands received.
- `UART_OVF`: UART receive-buffer overflows.
- `EEPROM_ERR`: corrupt EEPROM records or failed write verification.
- `RESET`: decoded MCU reset cause.
- `RESET_FLAGS`: original raw MCUSR flags.

## Notes

- A fully erased EEPROM on first startup is treated as `CAL=DEFAULT`, not as an EEPROM error.
- The boot counter uses a dedicated EEPROM dword and is updated only once per reset.
- Timer0 generates a 1 ms uptime tick. Global interrupts were already required by UART RX.

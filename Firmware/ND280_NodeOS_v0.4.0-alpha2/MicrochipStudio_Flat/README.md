# ND280 NodeOS v0.4.0-alpha1 — Architecture Refactor

This package refactors the validated `v0.3.1a` firmware into the NodeOS layered architecture **without intentionally changing acquisition, calibration, EEPROM, command, or packet behaviour**.

## Important

This is an architecture alpha. Keep `v0.3.1a` as the rollback reference until this build has been tested on the real board.

## Folder layout

```text
application/  main loop and node orchestration
hal/          ATmega328PB register-level UART0 and TWI1
 drivers/      TMAG5273 and hardware Board ID
services/     measurement, calibration, commands, CRC, UART transport
config/       compile-time configuration
docs/         architecture specification
```

## Microchip Studio setup

Create a GCC C Executable Project for **ATmega328PB** and add all `.c` files from:

- `application/`
- `hal/`
- `drivers/`
- `services/`

Add these include directories under project properties / compiler directories:

```text
application
hal
drivers
services
config
```

Alternatively use the `MicrochipStudio_Flat/` directory, which contains the same files in one folder and is simpler to import.

## Expected behaviour

- UART0 at 4800 baud
- bidirectional command receiver
- TWI1 on PE0/PE1
- TMAG5273A2 at 0x35
- ±266 mT range
- 10-sample average
- EEPROM calibration and CRC
- DIP-derived Board ID
- existing `$ND280,...` packets and command syntax

## Regression test

1. Confirm startup banner.
2. Confirm `TMAG_DEVICE_ID=0x06`.
3. Confirm continuous measurement packets.
4. Send `INFO`.
5. Send `CAL READ`.
6. Verify `CAL SAVE` and power-cycle persistence.
7. Compare magnetic and temperature values with v0.3.1a.

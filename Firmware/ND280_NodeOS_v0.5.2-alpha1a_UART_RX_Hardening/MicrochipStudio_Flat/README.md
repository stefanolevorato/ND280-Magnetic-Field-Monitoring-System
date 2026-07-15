# ND280 NodeOS v0.5.2-alpha1a

Minimal UART RX hardening release based directly on the validated v0.5.2-alpha1 watchdog firmware.

## Changes

- PD0/RXD0 internal pull-up enabled: UART idle remains HIGH if the Pico/adaptor TX is unplugged.
- Corrupted UART frames with FE0, DOR0, or UPE0 are discarded in the RX interrupt.
- No changes to TMAG acquisition, calibration, EEPROM, watchdog, diagnostics, packet format, or command execution.

## Microchip Studio

Use the `MicrochipStudio_Flat` directory. Create/select an ATmega328PB GCC C project, add all `.c` and `.h` files, then Clean, Rebuild, and Flash.

## Expected banner

`ND280 Magnetic Field Node v0.5.2-alpha1a`

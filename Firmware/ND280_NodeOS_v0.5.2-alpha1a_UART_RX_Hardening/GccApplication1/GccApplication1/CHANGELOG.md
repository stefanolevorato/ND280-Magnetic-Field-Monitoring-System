# Changelog

## v0.5.2-alpha1a

### Fixed
- Enabled the ATmega328PB internal pull-up on PD0/RXD0 so the UART input remains at the correct idle-high level when the external transmitter is disconnected.
- UART RX ISR now reads UCSR0A before UDR0 and discards bytes affected by framing, data-overrun, or parity errors.
- Preserved the validated v0.5.2-alpha1 measurement, watchdog, EEPROM, diagnostics, and command logic without modification.

### Baseline
This release is a minimal hardening patch derived directly from v0.5.2-alpha1. The withdrawn v0.5.2-alpha2 command-filter experiment is not included.

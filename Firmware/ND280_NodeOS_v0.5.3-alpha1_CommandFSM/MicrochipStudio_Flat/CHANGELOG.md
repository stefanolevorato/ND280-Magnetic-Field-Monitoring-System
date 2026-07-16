# Changelog

## v0.5.3-alpha1

### Changed
- Replaced the line-oriented UART RX buffer with a deterministic finite-state machine.
- Commands now require the `:` start marker.
- Maximum command length is 47 characters plus terminator.
- One command is dispatched per application service call.

### Robustness
- Non-command traffic is ignored.
- Printable ASCII only is accepted inside commands.
- Oversized or malformed commands are discarded until resynchronization.
- UART/parser errors are counted silently and cannot generate response storms.
- The validated measurement, TMAG, EEPROM, watchdog and calibration code is unchanged.

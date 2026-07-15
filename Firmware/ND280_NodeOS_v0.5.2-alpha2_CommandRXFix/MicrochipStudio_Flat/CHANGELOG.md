# Changelog

## v0.5.2-alpha2

### Fixed
- A continuous or echoed command stream can no longer trap `commands_process_pending()`.
- At most one received command is executed per application pass.
- Identical commands repeated within 1 second are suppressed.
- Watchdog is serviced after command execution.

### Added
- `UART_DUP` diagnostic counter for suppressed duplicate commands.

### Unchanged
- Measurement packet format and EEPROM calibration layout.

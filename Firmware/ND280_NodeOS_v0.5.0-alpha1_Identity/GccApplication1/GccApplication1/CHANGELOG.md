# Changelog

## NodeOS v0.5.0-alpha1 — Identity milestone

### Added
- Central `node_identity` service.
- Extended `INFO` response with NodeOS version, protocol version, board revision,
  hardware name, MCU, sensor, range, averaging, EEPROM structure version,
  calibration state, compilation date and compilation time.
- `IDENTITY` command as an alias for `INFO`.
- Boot identity summary.
- Centralized POST timings in `config.h`.

### Changed
- Firmware version updated to `0.5.0-alpha1`.
- LED POST timings use the visually validated values: 240 ms, 500 ms and 120 ms.

### Unchanged
- TMAG acquisition and ±266 mT conversion.
- EEPROM format and CRC.
- Calibration commands and data format.
- UART rate, TWI1 driver and measurement packet format.

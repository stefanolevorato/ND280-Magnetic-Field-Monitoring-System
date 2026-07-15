# Changelog

## 0.5.1-alpha1

### Added

- `diagnostics_service` with a Timer0-based millisecond uptime counter.
- Persistent EEPROM boot counter.
- Reset-cause capture from `MCUSR`.
- `DIAG` command and `$ND280DIAG` response.
- Measurement success/failure counters.
- TWI transaction error counter.
- UART command and RX-overflow counters.
- EEPROM validation/write-verification error counter.

### Changed

- Startup banner now reports reset cause and boot count.
- `HELP` now lists `DIAG`.

### Preserved

- TMAG configuration and ±266 mT scaling.
- EEPROM calibration format version 1.
- Existing `$ND280` measurement packet format.
- Status LED behaviour and visually validated POST timings.

# Changelog

## 0.5.2-alpha1

### Added
- Hardware watchdog enabled with a 4 second normal timeout.
- `watchdog_service` module.
- Controlled command `DIAG WDT TEST`.
- Persistent EEPROM watchdog-reset counter.
- `WDT_RESETS` field in `$ND280DIAG`.

### Preserved
- Measurement packet format.
- Calibration EEPROM structure.
- Status LED behaviour.
- UART and TWI1 configuration.

# Changelog

## 0.6.1

### Fixed
- Removed automatic 10-second DIAG polling.
- Diagnostics now refresh once on open and thereafter only on explicit user request.
- Avoids command duplication/queueing with Pico USB-UART bridge configurations.

# Changelog

## 0.6.0

### Added

- NodeOS diagnostics parser.
- Diagnostics Dashboard.
- Human-readable uptime.
- Node health indicator.
- Automatic ten-second diagnostics refresh.
- Confirmed watchdog reset test.
- DIAG quick-command button.

### Preserved

- Multi-node live plots.
- CSV logging.
- Calibration characterization.
- EEPROM calibration control.
- Bidirectional NodeOS command console.
- PyQt6 `Qt.PenStyle` compatibility.
- Fixed mT axis scaling.

# Changelog

## 0.5.0

### Added
- Calibration / EEPROM property editor.
- NodeOS command sequencing for offset, gain, noise, temperature, save and readback.
- Automatic readback verification after calibration writes.
- Factory-reset confirmation.

### Fixed
- Parser now accepts standalone `OK` and `ERROR` tokens in `$ND280RSP` messages.

### Preserved
- Multi-node live plotting.
- CSV logging.
- Characterization dialog.
- Command console and quick commands.
- Fixed mT plot-axis scaling.

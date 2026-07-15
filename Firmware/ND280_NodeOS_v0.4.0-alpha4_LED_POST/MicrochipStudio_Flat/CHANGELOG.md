# Changelog

## v0.4.0-alpha4

### Added
- Power-on self-test for PD5, PD6 and PD7.
- LED sequence: PD5, PD6, PD7, then all LEDs together.
- Automatic return to the established runtime LED meanings after POST.

### Preserved
- PD5 heartbeat behaviour after each published measurement.
- PD6 sensor-status behaviour.
- PD7 calibration/configuration-status behaviour.
- TMAG acquisition, EEPROM, commands and serial protocol.

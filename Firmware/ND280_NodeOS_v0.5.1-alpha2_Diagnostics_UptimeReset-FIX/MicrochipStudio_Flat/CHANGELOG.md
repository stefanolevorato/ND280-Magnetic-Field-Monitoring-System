# Changelog

## v0.5.1-alpha2

### Changed

- Replaced public `UPTIME_MS` with `UPTIME_S`.
- Extended 32-bit uptime rollover from 49.7 days to approximately 136 years.
- Captured and cleared `MCUSR` in the `.init3` startup section.
- Reset diagnostics now report all asserted causes instead of selecting only one.

### Preserved

- TMAG5273A2 acquisition and ±266 mT range.
- Calibration EEPROM layout and protocol.
- UART command protocol.
- Status LEDs and POST timings.
- Measurement packet format.

## v0.5.1-alpha2b
- Fixed missing reset-cause symbol in startup banner.

# Changelog

## v0.4.0-alpha2 - Control Panel integration baseline

### Changed
- Release documentation aligned with ND280 Monitor v0.4.0.
- The validated v0.4.0-alpha1 acquisition and command implementation is retained unchanged.

### Validated commands
- `HELP`
- `INFO`
- `CAL READ`
- `CAL OFFSET x y z`
- `CAL GAIN x y z`
- `CAL NOISE x y z`
- `CAL TEMP t`
- `CAL SAVE`
- `CAL RESET`

### Regression requirements
- Continuous `$ND280` output remains active while commands are processed.
- TMAG5273A2 remains configured for the 266 mT range.
- EEPROM states DEFAULT, DIRTY and VALID remain operational.

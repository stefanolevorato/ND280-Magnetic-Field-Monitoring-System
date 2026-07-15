# NodeOS v0.5.0-alpha1 Test Plan

1. Build for ATmega328PB using every `.c` and `.h` file in `MicrochipStudio_Flat`.
2. Verify LED POST timings: 240 ms per LED, 120 ms gaps, 500 ms all-on.
3. Verify normal runtime LED states are unchanged.
4. Confirm startup banner reports `0.5.0-alpha1` and build date/time.
5. Send `INFO` and verify a single `$ND280INFO` line containing:
   `ID`, `NODEOS`, `PROTO`, `BOARD_REV`, `HW`, `MCU`, `SENSOR`, `RANGE`,
   `AVG`, `EEPROM_VER`, `CAL`, `BUILD_DATE`, and `BUILD_TIME`.
6. Send `IDENTITY` and verify the same response.
7. Send `CAL READ`; verify EEPROM/calibration behaviour is unchanged.
8. Confirm normal `$ND280` measurement packets remain continuous.

# NodeOS v0.4.0-alpha2 test plan

1. Build for ATmega328PB and flash through AVRISP mkII.
2. Confirm startup banner and `TMAG_DEVICE_ID=0x06`.
3. Confirm sequential `$ND280` measurement packets.
4. Connect bidirectional 3.3 V UART at 4800 baud.
5. From ND280 Monitor v0.4.0 send:
   - HELP
   - INFO
   - CAL READ
6. Confirm the corresponding `$ND280RSP`, `$ND280INFO` and `$ND280CAL` responses.
7. Confirm measurements continue while commands are exchanged.
8. Test `CAL RESET` only after preserving any useful calibration data.

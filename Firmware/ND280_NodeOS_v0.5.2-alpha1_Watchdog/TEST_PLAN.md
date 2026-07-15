# Test Plan - NodeOS v0.5.2-alpha1

1. Build and flash from `MicrochipStudio_Flat`.
2. Confirm normal measurements continue for at least 5 minutes.
3. Send `DIAG`; verify `WDT_RESETS` is present.
4. Send `DIAG WDT TEST`.
5. Confirm acknowledgement appears before reset.
6. Confirm the node reboots in about one second.
7. Send `DIAG`; expected:
   - `RESET=WATCHDOG`
   - watchdog flag present in `RESET_FLAGS`
   - `WDT_RESETS` increased by one
   - measurements resume automatically
8. Perform an external reset and power cycle to verify existing reset diagnostics still pass.

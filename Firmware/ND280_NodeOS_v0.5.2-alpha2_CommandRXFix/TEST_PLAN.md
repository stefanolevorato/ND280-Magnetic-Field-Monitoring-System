# Test Plan - v0.5.2-alpha2

1. Flash from `MicrochipStudio_Flat`.
2. Verify startup version `0.5.2-alpha2`.
3. Send one `DIAG`; expect one response only.
4. Verify `MEAS_OK` and sequence continue increasing.
5. Send `DIAG` repeatedly faster than 1 Hz; duplicates should be suppressed and `UART_DUP` should increase.
6. Wait more than 1 second and send `DIAG` again; it must be accepted.
7. Run `DIAG WDT TEST`; watchdog reset behavior must remain valid.

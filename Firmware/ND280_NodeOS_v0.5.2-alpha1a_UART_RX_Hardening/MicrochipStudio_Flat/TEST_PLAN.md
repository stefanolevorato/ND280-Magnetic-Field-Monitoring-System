# Test Plan - v0.5.2-alpha1a

1. Flash from `MicrochipStudio_Flat` and verify the alpha1a banner.
2. With Pico GP0/TX connected, confirm normal measurements and one response per `DIAG` command.
3. Disconnect Pico GP0/TX, leaving PD0 otherwise unconnected. Confirm measurements remain plausible and the node does not reset repeatedly.
4. Reconnect GP0/TX and test `HELP`, `INFO`, `CAL READ`, and `DIAG`.
5. Run `DIAG WDT TEST`; verify one watchdog reset and automatic recovery.
6. Confirm `MEAS_FAIL=0`, `TWI_ERR=0`, and no repeated `UART_RX_OVERFLOW` messages under normal wiring.

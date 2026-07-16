# Test Plan - Command Parser FSM

1. Build for ATmega328PB and flash the node.
2. Confirm normal `$ND280` measurements remain plausible and continuous.
3. Send `DIAG` without prefix: expect no response.
4. Send `:DIAG`: expect exactly one `$ND280DIAG` response.
5. Send `:INFO`, `:HELP`, and `:CAL READ`: expect one response each.
6. Send `:DIAG WDT TEST`: expect one controlled watchdog reset.
7. Disconnect the external TX from PD0: measurements must continue normally.
8. Inject or paste `$ND280,...` into RX: it must be ignored.
9. Send a command longer than 47 characters: it must be discarded without reset or data corruption.
10. Run overnight logging and verify MEAS_FAIL=0, TWI_ERR=0 and no spontaneous watchdog resets.

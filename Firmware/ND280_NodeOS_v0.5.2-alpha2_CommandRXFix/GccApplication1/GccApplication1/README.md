# NodeOS v0.5.2-alpha2 - Command RX starvation fix

Use `MicrochipStudio_Flat` for the Microchip Studio project.

This release fixes repeated execution of a single command, observed with `DIAG`, which could starve measurement acquisition and eventually cause a watchdog reset.

The receiver now:

1. processes at most one complete line per application pass;
2. suppresses identical commands repeated within 1000 ms;
3. records suppressed duplicates as `UART_DUP`;
4. services the watchdog after command processing.

Expected result after one `DIAG` command: exactly one `$ND280DIAG` response, followed by normal measurement packets.

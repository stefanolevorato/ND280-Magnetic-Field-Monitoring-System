# ND280 Monitor 0.7.1 — Command Transport Fix

Compatible with **NodeOS v0.5.3-alpha1** and later.

## Fixed

All command sources now use one transport framing function:

- command entry field;
- HELP, INFO, CAL READ, CAL SAVE, DIAG and CAL RESET buttons;
- Diagnostics window;
- Watchdog test;
- Calibration / EEPROM dialog and command sequences.

The user writes or selects `DIAG`; the serial transport always sends:

```text
:DIAG\r\n
```

Entering `:DIAG` manually is also accepted and does not produce `::DIAG`.

## Console

The **Clear serial console** button only clears the visible text. It does not
stop logging, reset plots or alter node state.

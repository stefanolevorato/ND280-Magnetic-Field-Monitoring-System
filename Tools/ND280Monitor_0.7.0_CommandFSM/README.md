# ND280 Monitor 0.7.0 - Command FSM Support

This release is coordinated with NodeOS v0.5.3-alpha1 and its finite-state-machine command parser.

## Changes

- Commands entered by the user remain simple: `INFO`, `DIAG`, `CAL READ`, etc.
- The monitor automatically transmits the NodeOS framing prefix `:`.
- A command already beginning with `:` is not prefixed twice.
- All command paths use the same framing, including quick buttons, diagnostics, watchdog testing and calibration/EEPROM sequences.
- Added **Clear serial console**, which only clears the visible console and does not stop logging or alter node data.
- Preserves manual diagnostics refresh, multi-node plots, CSV logging and calibration controls.

## Expected firmware

NodeOS v0.5.3-alpha1 or later. The monitor sends:

```text
:HELP
:INFO
:DIAG
:CAL READ
```

while the interface displays the user's original command without the framing prefix.

## Run

Double-click `run_monitor.bat`, select the serial port at 4800 baud and press **Connect**.

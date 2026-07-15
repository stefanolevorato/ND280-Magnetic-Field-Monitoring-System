# ND280 Monitor 0.5.0 - Calibration Control Panel

This release adds direct control of the NodeOS calibration record and EEPROM.

## New functions

- Read calibration from the selected node.
- Edit X/Y/Z offset in mT.
- Edit X/Y/Z gain as a dimensionless multiplier.
- Edit stationary-noise sigma in mT.
- Edit calibration temperature in degrees Celsius.
- Apply values to node RAM.
- Apply, save to EEPROM, and automatically read back for verification.
- Factory-reset calibration with confirmation.
- Correct parsing of `$ND280RSP,OK,...` and `$ND280RSP,ERROR,...` messages.

## Run

```bat
run_monitor.bat
```

Connect at 4800 baud, select the node, then open **Calibration / EEPROM...**.

## Recommended first test

1. Click **Read from node**.
2. Confirm DEFAULT or VALID values are displayed.
3. Enter unmistakable temporary values, for example offsets 0.10, 0.20, 0.30 mT.
4. Click **Apply + save EEPROM**.
5. Confirm the dialog reports successful verification and state VALID.
6. Power-cycle the node and click **Read from node** again.
7. Restore defaults or real calibration values afterward.

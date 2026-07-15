# ND280 Magnetic Field Monitor 0.3.0

This release adds a software-only calibration and noise-characterization workflow.

## New calibration workflow

Open **Calibration...** after connecting and selecting a node.

The six-position procedure estimates sensor offsets without a calibrated gaussmeter:

1. Align a stable ambient field along sensor **+X**, capture X+.
2. Rotate/reposition the board so the same field points along **-X**, capture X-.
3. Repeat for +Y/-Y and +Z/-Z.
4. Optionally acquire stationary-noise samples with the board fixed.
5. Click **Compute calibration**.
6. Save the result as JSON.

The offset for each axis is calculated as:

`offset = (mean_plus + mean_minus) / 2`

The gains remain fixed at 1.0. Absolute gain calibration still requires a traceable field reference or a calibrated coil/gaussmeter.

Calibration data are not yet written to the node EEPROM and are not automatically applied to live data. This avoids altering measurements before the EEPROM format and node command protocol are finalized.

## Installation

Run `run_monitor.bat`. Required packages are installed from `requirements.txt`.

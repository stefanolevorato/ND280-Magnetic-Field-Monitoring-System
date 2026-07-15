# ND280 Magnetic Field Monitor 0.2.1

Windows/PyQt6 application for live monitoring of ND280 magnetic-field nodes.

## Main functions

- Serial acquisition from the node protocol `$ND280,...`
- Multi-node selection by Board ID
- Live plots of Bx, By, Bz and temperature
- CSV logging
- Sequence and missing-packet counters
- Adjustable history length

## Fix in 0.2.1

Pyqtgraph automatic SI-prefix scaling is disabled on the vertical axes. Values
received and displayed in mT now use exactly the same numerical scale on the
magnetic-field graph. For example, `BX=-0.20` is plotted near `-0.20 mT`, not
near `-200` on an automatically rescaled axis.

## Run on Windows

Double-click `run_monitor.bat`, or run:

```bat
py -m pip install -r requirements.txt
py -m nd280_monitor.main
```

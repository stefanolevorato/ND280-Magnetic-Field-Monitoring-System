# ND280 Magnetic Field Monitor 0.1.0

Windows application for reading, plotting, and logging packets from the ND280 magnetic-field node.

Expected packet format:

```text
$ND280,VER=0.2.1,ID=31,SEQ=125,AVG=10,T=30.25,BX=-0.02,BY=-0.04,BZ=-0.03,STATUS=0x13
```

## Quick start

1. Install Python 3.11 or newer from python.org and enable **Add Python to PATH**.
2. Connect the Raspberry Pi Pico or USB-UART bridge.
3. Double-click `run_monitor.bat`.
4. Select the COM port and 4800 baud.
5. Press **Connect**.
6. Press **Start logging** to create a timestamped CSV file.

## Features

- Lists available Windows COM ports.
- Parses `$ND280` packets.
- Live plots of Bx, By, Bz, and temperature.
- Displays board ID, firmware version, sequence, averaging, and status.
- Detects missing sequence numbers.
- Saves received packets as CSV.
- Ignores startup text and malformed serial lines.

CSV files are written into the `logs` directory.

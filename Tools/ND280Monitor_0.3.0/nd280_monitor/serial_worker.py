from __future__ import annotations

import serial
from PyQt6.QtCore import QThread, pyqtSignal


class SerialWorker(QThread):
    line_received = pyqtSignal(str)
    serial_error = pyqtSignal(str)
    connected = pyqtSignal()
    disconnected = pyqtSignal()

    def __init__(self, port: str, baudrate: int) -> None:
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self._running = True
        self._serial: serial.Serial | None = None

    def run(self) -> None:
        try:
            self._serial = serial.Serial(
                self.port,
                self.baudrate,
                timeout=0.2,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
            )
            self.connected.emit()
            while self._running:
                raw = self._serial.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace").strip()
                if line:
                    self.line_received.emit(line)
        except Exception as exc:
            self.serial_error.emit(str(exc))
        finally:
            if self._serial is not None and self._serial.is_open:
                self._serial.close()
            self.disconnected.emit()

    def stop(self) -> None:
        self._running = False
        self.wait(1500)

from __future__ import annotations

import sys
from collections import deque
from pathlib import Path
from time import monotonic

import pyqtgraph as pg
from PyQt6.QtCore import QTimer, Qt
from PyQt6.QtWidgets import (
    QApplication,
    QComboBox,
    QFormLayout,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QPlainTextEdit,
    QSpinBox,
    QVBoxLayout,
    QWidget,
)
from serial.tools import list_ports

from .logger import CSVLogger
from .parser import ND280Packet, parse_packet
from .serial_worker import SerialWorker


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("ND280 Magnetic Field Monitor 0.1.0")
        self.resize(1250, 850)

        self.worker: SerialWorker | None = None
        self.logger = CSVLogger(Path(__file__).resolve().parent.parent / "logs")
        self.start_time = monotonic()
        self.last_sequence: dict[int, int] = {}
        self.packet_count = 0
        self.missing_count = 0
        self.max_points = 600

        self.times: deque[float] = deque(maxlen=self.max_points)
        self.bx: deque[float] = deque(maxlen=self.max_points)
        self.by: deque[float] = deque(maxlen=self.max_points)
        self.bz: deque[float] = deque(maxlen=self.max_points)
        self.temp: deque[float] = deque(maxlen=self.max_points)

        self._build_ui()
        self.refresh_ports()

        self.plot_timer = QTimer(self)
        self.plot_timer.timeout.connect(self.update_plots)
        self.plot_timer.start(200)

    def _build_ui(self) -> None:
        central = QWidget()
        self.setCentralWidget(central)
        root = QVBoxLayout(central)

        connection = QHBoxLayout()
        self.port_combo = QComboBox()
        self.baud_combo = QComboBox()
        self.baud_combo.addItems(["4800", "9600", "19200", "38400", "115200"])
        self.baud_combo.setCurrentText("4800")
        self.refresh_button = QPushButton("Refresh ports")
        self.connect_button = QPushButton("Connect")
        self.log_button = QPushButton("Start logging")
        self.log_button.setEnabled(False)
        connection.addWidget(QLabel("Port"))
        connection.addWidget(self.port_combo)
        connection.addWidget(QLabel("Baud"))
        connection.addWidget(self.baud_combo)
        connection.addWidget(self.refresh_button)
        connection.addWidget(self.connect_button)
        connection.addWidget(self.log_button)
        connection.addStretch()
        root.addLayout(connection)

        self.refresh_button.clicked.connect(self.refresh_ports)
        self.connect_button.clicked.connect(self.toggle_connection)
        self.log_button.clicked.connect(self.toggle_logging)

        status_box = QGroupBox("Current node")
        status_layout = QFormLayout(status_box)
        self.node_label = QLabel("-")
        self.firmware_label = QLabel("-")
        self.seq_label = QLabel("-")
        self.avg_label = QLabel("-")
        self.status_label = QLabel("-")
        self.packet_label = QLabel("0")
        self.missing_label = QLabel("0")
        self.temp_label = QLabel("-")
        self.bx_label = QLabel("-")
        self.by_label = QLabel("-")
        self.bz_label = QLabel("-")
        status_layout.addRow("Board ID", self.node_label)
        status_layout.addRow("Firmware", self.firmware_label)
        status_layout.addRow("Sequence", self.seq_label)
        status_layout.addRow("Average", self.avg_label)
        status_layout.addRow("Status", self.status_label)
        status_layout.addRow("Packets", self.packet_label)
        status_layout.addRow("Missing", self.missing_label)
        status_layout.addRow("Temperature", self.temp_label)
        status_layout.addRow("Bx", self.bx_label)
        status_layout.addRow("By", self.by_label)
        status_layout.addRow("Bz", self.bz_label)

        plot_container = QWidget()
        plots = QGridLayout(plot_container)
        self.field_plot = pg.PlotWidget(title="Magnetic field")
        self.field_plot.setLabel("left", "B", units="mT")
        self.field_plot.setLabel("bottom", "Time", units="s")
        self.field_plot.showGrid(x=True, y=True)
        self.bx_curve = self.field_plot.plot(name="Bx", pen=pg.mkPen(width=2))
        self.by_curve = self.field_plot.plot(name="By", pen=pg.mkPen(width=2, style=Qt.PenStyle.DashLine))
        self.bz_curve = self.field_plot.plot(name="Bz", pen=pg.mkPen(width=2, style=Qt.PenStyle.DotLine))
        self.field_plot.addLegend()

        self.temp_plot = pg.PlotWidget(title="Temperature")
        self.temp_plot.setLabel("left", "Temperature", units="C")
        self.temp_plot.setLabel("bottom", "Time", units="s")
        self.temp_plot.showGrid(x=True, y=True)
        self.temp_curve = self.temp_plot.plot(pen=pg.mkPen(width=2))

        plots.addWidget(self.field_plot, 0, 0)
        plots.addWidget(self.temp_plot, 1, 0)

        center = QHBoxLayout()
        center.addWidget(status_box, 0)
        center.addWidget(plot_container, 1)
        root.addLayout(center, 1)

        self.console = QPlainTextEdit()
        self.console.setReadOnly(True)
        self.console.document().setMaximumBlockCount(500)
        root.addWidget(QLabel("Serial console"))
        root.addWidget(self.console, 0)

        self.statusBar().showMessage("Disconnected")

    def refresh_ports(self) -> None:
        current = self.port_combo.currentData()
        self.port_combo.clear()
        for port in list_ports.comports():
            self.port_combo.addItem(f"{port.device} - {port.description}", port.device)
        if current:
            index = self.port_combo.findData(current)
            if index >= 0:
                self.port_combo.setCurrentIndex(index)

    def toggle_connection(self) -> None:
        if self.worker is not None:
            self.disconnect_serial()
            return
        port = self.port_combo.currentData()
        if not port:
            QMessageBox.warning(self, "No port", "Select a COM port first.")
            return
        baud = int(self.baud_combo.currentText())
        self.worker = SerialWorker(port, baud)
        self.worker.line_received.connect(self.handle_line)
        self.worker.serial_error.connect(self.handle_serial_error)
        self.worker.connected.connect(self.handle_connected)
        self.worker.disconnected.connect(self.handle_disconnected)
        self.worker.start()
        self.statusBar().showMessage(f"Opening {port} at {baud} baud...")

    def disconnect_serial(self) -> None:
        if self.worker is not None:
            worker = self.worker
            self.worker = None
            worker.stop()
        if self.logger.active:
            self.logger.stop()
            self.log_button.setText("Start logging")

    def handle_connected(self) -> None:
        self.connect_button.setText("Disconnect")
        self.log_button.setEnabled(True)
        self.statusBar().showMessage("Connected")

    def handle_disconnected(self) -> None:
        self.worker = None
        self.connect_button.setText("Connect")
        self.log_button.setEnabled(False)
        self.statusBar().showMessage("Disconnected")

    def handle_serial_error(self, message: str) -> None:
        QMessageBox.critical(self, "Serial error", message)

    def toggle_logging(self) -> None:
        if self.logger.active:
            path = self.logger.path
            self.logger.stop()
            self.log_button.setText("Start logging")
            self.statusBar().showMessage(f"Logging stopped: {path}")
        else:
            path = self.logger.start()
            self.log_button.setText("Stop logging")
            self.statusBar().showMessage(f"Logging to {path}")

    def handle_line(self, line: str) -> None:
        self.console.appendPlainText(line)
        packet = parse_packet(line)
        if packet is None:
            return
        self.handle_packet(packet)

    def handle_packet(self, packet: ND280Packet) -> None:
        self.packet_count += 1
        previous = self.last_sequence.get(packet.board_id)
        if previous is not None:
            expected = (previous + 1) & 0xFFFFFFFF
            if packet.sequence != expected:
                gap = (packet.sequence - expected) & 0xFFFFFFFF
                if gap < 1000000:
                    self.missing_count += gap
        self.last_sequence[packet.board_id] = packet.sequence

        now = monotonic() - self.start_time
        self.times.append(now)
        self.bx.append(packet.bx_mt)
        self.by.append(packet.by_mt)
        self.bz.append(packet.bz_mt)
        self.temp.append(packet.temperature_c)

        self.node_label.setText(str(packet.board_id))
        self.firmware_label.setText(packet.firmware)
        self.seq_label.setText(str(packet.sequence))
        self.avg_label.setText(str(packet.average))
        self.status_label.setText(f"0x{packet.status:02X}")
        self.packet_label.setText(str(self.packet_count))
        self.missing_label.setText(str(self.missing_count))
        self.temp_label.setText(f"{packet.temperature_c:.2f} C")
        self.bx_label.setText(f"{packet.bx_mt:.2f} mT")
        self.by_label.setText(f"{packet.by_mt:.2f} mT")
        self.bz_label.setText(f"{packet.bz_mt:.2f} mT")

        if self.logger.active:
            self.logger.write(packet)

    def update_plots(self) -> None:
        if not self.times:
            return
        x = list(self.times)
        self.bx_curve.setData(x, list(self.bx))
        self.by_curve.setData(x, list(self.by))
        self.bz_curve.setData(x, list(self.bz))
        self.temp_curve.setData(x, list(self.temp))

    def closeEvent(self, event) -> None:  # noqa: N802
        self.disconnect_serial()
        event.accept()


def main() -> int:
    app = QApplication(sys.argv)
    pg.setConfigOptions(antialias=True)
    window = MainWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())

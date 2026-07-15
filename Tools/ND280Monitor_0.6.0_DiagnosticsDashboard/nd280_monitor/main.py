from __future__ import annotations

import sys
from collections import deque
from dataclasses import dataclass, field
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
    QLineEdit,
    QMessageBox,
    QPushButton,
    QPlainTextEdit,
    QSpinBox,
    QVBoxLayout,
    QWidget,
)
from serial.tools import list_ports

from .calibration import CalibrationDialog
from .calibration_control import CalibrationControlDialog
from .diagnostics_dashboard import DiagnosticsDashboard
from .logger import CSVLogger
from .parser import (
    CalibrationMessage,
    DiagnosticMessage,
    InfoMessage,
    ND280Packet,
    ResponseMessage,
    parse_message,
)
from .serial_worker import SerialWorker


class CommandLineEdit(QLineEdit):
    def __init__(self) -> None:
        super().__init__()
        self._history: list[str] = []
        self._history_index = 0

    def add_to_history(self, command: str) -> None:
        command = command.strip()
        if not command:
            return
        if not self._history or self._history[-1] != command:
            self._history.append(command)
        self._history_index = len(self._history)

    def keyPressEvent(self, event) -> None:  # noqa: N802
        if event.key() == Qt.Key.Key_Up:
            if self._history and self._history_index > 0:
                self._history_index -= 1
                self.setText(self._history[self._history_index])
            return
        if event.key() == Qt.Key.Key_Down:
            if self._history_index < len(self._history) - 1:
                self._history_index += 1
                self.setText(self._history[self._history_index])
            else:
                self._history_index = len(self._history)
                self.clear()
            return
        super().keyPressEvent(event)


@dataclass
class NodeState:
    board_id: int
    max_points: int
    firmware: str = "-"
    sequence: int = 0
    average: int = 0
    status: int = 0
    packet_count: int = 0
    missing_count: int = 0
    last_sequence: int | None = None
    temperature_c: float = 0.0
    bx_mt: float = 0.0
    by_mt: float = 0.0
    bz_mt: float = 0.0
    times: deque[float] = field(init=False)
    bx: deque[float] = field(init=False)
    by: deque[float] = field(init=False)
    bz: deque[float] = field(init=False)
    temp: deque[float] = field(init=False)

    def __post_init__(self) -> None:
        self._create_buffers(self.max_points)

    def _create_buffers(self, max_points: int) -> None:
        self.max_points = max_points
        self.times = deque(maxlen=max_points)
        self.bx = deque(maxlen=max_points)
        self.by = deque(maxlen=max_points)
        self.bz = deque(maxlen=max_points)
        self.temp = deque(maxlen=max_points)

    def resize_history(self, max_points: int) -> None:
        if max_points == self.max_points:
            return

        old_times = list(self.times)[-max_points:]
        old_bx = list(self.bx)[-max_points:]
        old_by = list(self.by)[-max_points:]
        old_bz = list(self.bz)[-max_points:]
        old_temp = list(self.temp)[-max_points:]

        self._create_buffers(max_points)
        self.times.extend(old_times)
        self.bx.extend(old_bx)
        self.by.extend(old_by)
        self.bz.extend(old_bz)
        self.temp.extend(old_temp)

    def update(self, packet: ND280Packet, time_s: float) -> None:
        if self.last_sequence is not None:
            expected = (self.last_sequence + 1) & 0xFFFFFFFF
            if packet.sequence != expected:
                gap = (packet.sequence - expected) & 0xFFFFFFFF
                if gap < 1_000_000:
                    self.missing_count += gap

        self.last_sequence = packet.sequence
        self.sequence = packet.sequence
        self.firmware = packet.firmware
        self.average = packet.average
        self.status = packet.status
        self.temperature_c = packet.temperature_c
        self.bx_mt = packet.bx_mt
        self.by_mt = packet.by_mt
        self.bz_mt = packet.bz_mt
        self.packet_count += 1

        self.times.append(time_s)
        self.bx.append(packet.bx_mt)
        self.by.append(packet.by_mt)
        self.bz.append(packet.bz_mt)
        self.temp.append(packet.temperature_c)


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("ND280 Magnetic Field Monitor 0.6.0")
        self.resize(1250, 850)

        self.worker: SerialWorker | None = None
        self.logger = CSVLogger(Path(__file__).resolve().parent.parent / "logs")
        self.start_time = monotonic()

        self.nodes: dict[int, NodeState] = {}
        self.selected_node_id: int | None = None
        self.max_points = 600
        self.calibration_dialog: CalibrationDialog | None = None
        self.calibration_control_dialog: CalibrationControlDialog | None = None
        self.diagnostics_dashboard: DiagnosticsDashboard | None = None

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
        self.calibration_button = QPushButton("Characterization...")
        self.calibration_control_button = QPushButton("Calibration / EEPROM...")
        self.diagnostics_button = QPushButton("Diagnostics...")

        self.node_combo = QComboBox()
        self.node_combo.setMinimumWidth(100)
        self.node_combo.currentIndexChanged.connect(self.change_selected_node)

        self.history_spin = QSpinBox()
        self.history_spin.setRange(100, 10000)
        self.history_spin.setSingleStep(100)
        self.history_spin.setValue(self.max_points)
        self.history_spin.valueChanged.connect(self.change_history_length)

        connection.addWidget(QLabel("Port"))
        connection.addWidget(self.port_combo)
        connection.addWidget(QLabel("Baud"))
        connection.addWidget(self.baud_combo)
        connection.addWidget(self.refresh_button)
        connection.addWidget(self.connect_button)
        connection.addWidget(self.log_button)
        connection.addWidget(self.calibration_button)
        connection.addWidget(self.calibration_control_button)
        connection.addWidget(self.diagnostics_button)
        connection.addSpacing(20)
        connection.addWidget(QLabel("Node"))
        connection.addWidget(self.node_combo)
        connection.addWidget(QLabel("History"))
        connection.addWidget(self.history_spin)
        connection.addStretch()
        root.addLayout(connection)

        self.refresh_button.clicked.connect(self.refresh_ports)
        self.connect_button.clicked.connect(self.toggle_connection)
        self.log_button.clicked.connect(self.toggle_logging)
        self.calibration_button.clicked.connect(self.open_calibration)
        self.calibration_control_button.clicked.connect(self.open_calibration_control)
        self.diagnostics_button.clicked.connect(self.open_diagnostics)

        status_box = QGroupBox("Selected node")
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
        # Keep the numerical axis in mT. Pyqtgraph otherwise applies an
        # automatic SI prefix and may display -0.20 mT as -200 umT/mmT-like
        # tick values, which is confusing when the label still refers to mT.
        self.field_plot.getAxis("left").enableAutoSIPrefix(False)
        self.field_plot.setLabel("bottom", "Time", units="s")
        self.field_plot.showGrid(x=True, y=True)
        self.bx_curve = self.field_plot.plot(name="Bx", pen=pg.mkPen(width=2))
        self.by_curve = self.field_plot.plot(
            name="By",
            pen=pg.mkPen(width=2, style=Qt.PenStyle.DashLine),
        )
        self.bz_curve = self.field_plot.plot(
            name="Bz",
            pen=pg.mkPen(width=2, style=Qt.PenStyle.DotLine),
        )
        self.field_plot.addLegend()

        self.temp_plot = pg.PlotWidget(title="Temperature")
        self.temp_plot.setLabel("left", "Temperature", units="C")
        self.temp_plot.getAxis("left").enableAutoSIPrefix(False)
        self.temp_plot.setLabel("bottom", "Time", units="s")
        self.temp_plot.showGrid(x=True, y=True)
        self.temp_curve = self.temp_plot.plot(pen=pg.mkPen(width=2))

        plots.addWidget(self.field_plot, 0, 0)
        plots.addWidget(self.temp_plot, 1, 0)

        center = QHBoxLayout()
        center.addWidget(status_box, 0)
        center.addWidget(plot_container, 1)
        root.addLayout(center, 1)

        control_box = QGroupBox("NodeOS control panel")
        control_layout = QVBoxLayout(control_box)

        quick_commands = QHBoxLayout()
        for caption, command in (
            ("HELP", "HELP"),
            ("INFO", "INFO"),
            ("CAL READ", "CAL READ"),
            ("CAL SAVE", "CAL SAVE"),
            ("DIAG", "DIAG"),
        ):
            button = QPushButton(caption)
            button.clicked.connect(
                lambda checked=False, value=command: self.send_command(value)
            )
            quick_commands.addWidget(button)

        self.cal_reset_button = QPushButton("CAL RESET")
        self.cal_reset_button.clicked.connect(self.confirm_cal_reset)
        quick_commands.addWidget(self.cal_reset_button)
        quick_commands.addStretch()
        control_layout.addLayout(quick_commands)

        command_row = QHBoxLayout()
        self.command_input = CommandLineEdit()
        self.command_input.setPlaceholderText("Enter NodeOS command, e.g. INFO")
        self.command_input.returnPressed.connect(self.send_command_from_input)
        self.send_button = QPushButton("Send")
        self.send_button.clicked.connect(self.send_command_from_input)
        command_row.addWidget(QLabel(">"))
        command_row.addWidget(self.command_input, 1)
        command_row.addWidget(self.send_button)
        control_layout.addLayout(command_row)

        response_layout = QFormLayout()
        self.info_response_label = QLabel("-")
        self.cal_response_label = QLabel("-")
        self.rsp_response_label = QLabel("-")
        self.info_response_label.setWordWrap(True)
        self.cal_response_label.setWordWrap(True)
        self.rsp_response_label.setWordWrap(True)
        response_layout.addRow("Last INFO", self.info_response_label)
        response_layout.addRow("Last CAL", self.cal_response_label)
        response_layout.addRow("Last response", self.rsp_response_label)
        control_layout.addLayout(response_layout)
        root.addWidget(control_box)

        self.console = QPlainTextEdit()
        self.console.setReadOnly(True)
        self.console.document().setMaximumBlockCount(500)
        root.addWidget(QLabel("Serial console"))
        root.addWidget(self.console, 0)

        self.set_command_controls_enabled(False)
        self.statusBar().showMessage("Disconnected")

    def refresh_ports(self) -> None:
        current = self.port_combo.currentData()
        self.port_combo.clear()
        for port in list_ports.comports():
            self.port_combo.addItem(
                f"{port.device} - {port.description}",
                port.device,
            )
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
        self.set_command_controls_enabled(True)
        self.statusBar().showMessage("Connected")

    def handle_disconnected(self) -> None:
        self.worker = None
        self.connect_button.setText("Connect")
        self.log_button.setEnabled(False)
        self.set_command_controls_enabled(False)
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
        message = parse_message(line)
        if message is None:
            return
        if isinstance(message, ND280Packet):
            self.handle_packet(message)
        elif isinstance(message, InfoMessage):
            self.info_response_label.setText(self.format_fields(message.fields))
        elif isinstance(message, CalibrationMessage):
            self.cal_response_label.setText(self.format_fields(message.fields))
            if self.calibration_control_dialog is not None:
                self.calibration_control_dialog.consume_calibration(message)
        elif isinstance(message, DiagnosticMessage):
            if self.diagnostics_dashboard is not None:
                self.diagnostics_dashboard.consume_diagnostic(message)
            self.rsp_response_label.setText(self.format_fields(message.fields))
        elif isinstance(message, ResponseMessage):
            formatted = self.format_fields(message.fields)
            if message.flags:
                flag_text = ", ".join(sorted(message.flags))
                formatted = f"{flag_text}: {formatted}" if formatted != "-" else flag_text
            self.rsp_response_label.setText(formatted)
            if self.calibration_control_dialog is not None:
                self.calibration_control_dialog.consume_response(message)

    def set_command_controls_enabled(self, enabled: bool) -> None:
        self.command_input.setEnabled(enabled)
        self.send_button.setEnabled(enabled)
        self.cal_reset_button.setEnabled(enabled)

    @staticmethod
    def format_fields(fields: dict[str, str]) -> str:
        return ", ".join(f"{key}={value}" for key, value in fields.items()) or "-"

    def send_command_from_input(self) -> None:
        command = self.command_input.text().strip()
        if self.send_command(command):
            self.command_input.add_to_history(command)
            self.command_input.clear()

    def send_command(self, command: str) -> bool:
        if self.worker is None:
            QMessageBox.warning(self, "Not connected", "Connect to a node first.")
            return False
        command = command.strip()
        if not command:
            return False
        if not self.worker.send_line(command):
            QMessageBox.warning(self, "Command not sent", "The serial port is not ready.")
            return False
        self.console.appendPlainText(f"> {command}")
        self.statusBar().showMessage(f"Sent: {command}", 3000)
        return True

    def confirm_cal_reset(self) -> None:
        answer = QMessageBox.question(
            self,
            "Reset calibration",
            "Reset calibration values to factory defaults?\n"
            "The change remains in RAM until CAL SAVE is issued.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if answer == QMessageBox.StandardButton.Yes:
            self.send_command("CAL RESET")

    def handle_packet(self, packet: ND280Packet) -> None:
        node = self.nodes.get(packet.board_id)
        if node is None:
            node = NodeState(packet.board_id, self.max_points)
            self.nodes[packet.board_id] = node
            self.node_combo.addItem(str(packet.board_id), packet.board_id)

            if self.selected_node_id is None:
                self.selected_node_id = packet.board_id
                self.node_combo.setCurrentIndex(
                    self.node_combo.findData(packet.board_id)
                )

        now = monotonic() - self.start_time
        node.update(packet, now)

        if packet.board_id == self.selected_node_id:
            self.refresh_selected_node()

        if self.calibration_dialog is not None:
            self.calibration_dialog.consume_packet(packet)

        if self.logger.active:
            self.logger.write(packet)


    def open_calibration(self) -> None:
        if self.calibration_dialog is None:
            directory = Path(__file__).resolve().parent.parent / "calibrations"
            self.calibration_dialog = CalibrationDialog(
                directory,
                lambda: self.selected_node_id,
                self,
            )
        self.calibration_dialog.show()
        self.calibration_dialog.raise_()
        self.calibration_dialog.activateWindow()

    def open_calibration_control(self) -> None:
        if self.calibration_control_dialog is None:
            self.calibration_control_dialog = CalibrationControlDialog(
                lambda: self.selected_node_id,
                self.send_command,
                self,
            )
        self.calibration_control_dialog.show()
        self.calibration_control_dialog.raise_()
        self.calibration_control_dialog.activateWindow()

    def open_diagnostics(self) -> None:
        if self.diagnostics_dashboard is None:
            self.diagnostics_dashboard = DiagnosticsDashboard(
                self.send_command,
                self,
            )
        self.diagnostics_dashboard.show()
        self.diagnostics_dashboard.raise_()
        self.diagnostics_dashboard.activateWindow()

    def change_selected_node(self) -> None:
        board_id = self.node_combo.currentData()
        if board_id is None:
            return

        self.selected_node_id = int(board_id)
        self.refresh_selected_node()
        self.update_plots()

    def change_history_length(self, value: int) -> None:
        self.max_points = value
        for node in self.nodes.values():
            node.resize_history(value)
        self.update_plots()

    def refresh_selected_node(self) -> None:
        if self.selected_node_id is None:
            return

        node = self.nodes.get(self.selected_node_id)
        if node is None:
            return

        self.node_label.setText(str(node.board_id))
        self.firmware_label.setText(node.firmware)
        self.seq_label.setText(str(node.sequence))
        self.avg_label.setText(str(node.average))
        self.status_label.setText(f"0x{node.status:02X}")
        self.packet_label.setText(str(node.packet_count))
        self.missing_label.setText(str(node.missing_count))
        self.temp_label.setText(f"{node.temperature_c:.2f} C")
        self.bx_label.setText(f"{node.bx_mt:.2f} mT")
        self.by_label.setText(f"{node.by_mt:.2f} mT")
        self.bz_label.setText(f"{node.bz_mt:.2f} mT")

    def update_plots(self) -> None:
        if self.selected_node_id is None:
            return

        node = self.nodes.get(self.selected_node_id)
        if node is None or not node.times:
            return

        x = list(node.times)
        self.bx_curve.setData(x, list(node.bx))
        self.by_curve.setData(x, list(node.by))
        self.bz_curve.setData(x, list(node.bz))
        self.temp_curve.setData(x, list(node.temp))

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

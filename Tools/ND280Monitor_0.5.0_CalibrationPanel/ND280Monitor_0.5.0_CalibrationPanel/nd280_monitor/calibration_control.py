from __future__ import annotations

from collections.abc import Callable

from PyQt6.QtCore import QTimer
from PyQt6.QtWidgets import (
    QDialog,
    QDoubleSpinBox,
    QFormLayout,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QMessageBox,
    QPushButton,
    QVBoxLayout,
)

from .parser import CalibrationMessage, ResponseMessage


class CalibrationControlDialog(QDialog):
    """Read, edit and persist NodeOS calibration values."""

    def __init__(
        self,
        selected_node: Callable[[], int | None],
        send_command: Callable[[str], bool],
        parent=None,
    ) -> None:
        super().__init__(parent)
        self.setWindowTitle("NodeOS calibration / EEPROM")
        self.resize(650, 520)

        self.selected_node = selected_node
        self.send_command = send_command
        self._command_queue: list[str] = []
        self._expected_values: dict[str, int] | None = None

        self._build_ui()

        self.command_timer = QTimer(self)
        self.command_timer.setSingleShot(True)
        self.command_timer.timeout.connect(self._send_next_queued_command)

    def _build_ui(self) -> None:
        root = QVBoxLayout(self)

        status_box = QGroupBox("Calibration record")
        status = QFormLayout(status_box)
        self.node_label = QLabel("-")
        self.state_label = QLabel("-")
        self.version_label = QLabel("-")
        self.counter_label = QLabel("-")
        status.addRow("Selected node", self.node_label)
        status.addRow("EEPROM state", self.state_label)
        status.addRow("Record version", self.version_label)
        status.addRow("Save counter", self.counter_label)
        root.addWidget(status_box)

        values_box = QGroupBox("Calibration values")
        grid = QGridLayout(values_box)
        grid.addWidget(QLabel("Axis"), 0, 0)
        grid.addWidget(QLabel("Offset [mT]"), 0, 1)
        grid.addWidget(QLabel("Gain"), 0, 2)
        grid.addWidget(QLabel("Noise sigma [mT]"), 0, 3)

        self.offset: list[QDoubleSpinBox] = []
        self.gain: list[QDoubleSpinBox] = []
        self.noise: list[QDoubleSpinBox] = []

        for row, axis in enumerate(("X", "Y", "Z"), start=1):
            grid.addWidget(QLabel(axis), row, 0)

            offset = QDoubleSpinBox()
            offset.setRange(-266.0, 266.0)
            offset.setDecimals(2)
            offset.setSingleStep(0.01)
            offset.setSuffix(" mT")
            grid.addWidget(offset, row, 1)
            self.offset.append(offset)

            gain = QDoubleSpinBox()
            gain.setRange(0.000001, 10.0)
            gain.setDecimals(6)
            gain.setSingleStep(0.000001)
            gain.setValue(1.0)
            grid.addWidget(gain, row, 2)
            self.gain.append(gain)

            noise = QDoubleSpinBox()
            noise.setRange(0.0, 266.0)
            noise.setDecimals(4)
            noise.setSingleStep(0.0001)
            noise.setSuffix(" mT")
            grid.addWidget(noise, row, 3)
            self.noise.append(noise)

        self.temperature = QDoubleSpinBox()
        self.temperature.setRange(-55.0, 150.0)
        self.temperature.setDecimals(2)
        self.temperature.setSingleStep(0.01)
        self.temperature.setSuffix(" C")
        grid.addWidget(QLabel("Calibration temperature"), 4, 0, 1, 2)
        grid.addWidget(self.temperature, 4, 2, 1, 2)
        root.addWidget(values_box)

        actions = QHBoxLayout()
        self.read_button = QPushButton("Read from node")
        self.apply_button = QPushButton("Apply to RAM")
        self.save_button = QPushButton("Apply + save EEPROM")
        self.reset_button = QPushButton("Factory reset")
        self.close_button = QPushButton("Close")
        actions.addWidget(self.read_button)
        actions.addWidget(self.apply_button)
        actions.addWidget(self.save_button)
        actions.addWidget(self.reset_button)
        actions.addStretch()
        actions.addWidget(self.close_button)
        root.addLayout(actions)

        self.progress_label = QLabel("Ready")
        self.progress_label.setWordWrap(True)
        root.addWidget(self.progress_label)

        note = QLabel(
            "Protocol scaling: offset = 0.01 mT/count, gain = ppm, "
            "noise = 0.0001 mT/count, calibration temperature = 0.01 C/count."
        )
        note.setWordWrap(True)
        root.addWidget(note)

        self.read_button.clicked.connect(self.read_from_node)
        self.apply_button.clicked.connect(self.apply_to_ram)
        self.save_button.clicked.connect(self.apply_and_save)
        self.reset_button.clicked.connect(self.confirm_reset)
        self.close_button.clicked.connect(self.close)

    def showEvent(self, event) -> None:  # noqa: N802
        node_id = self.selected_node()
        self.node_label.setText(str(node_id) if node_id is not None else "-")
        super().showEvent(event)

    def read_from_node(self) -> None:
        if self._require_node():
            self.progress_label.setText("Requesting calibration record...")
            self.send_command("CAL READ")

    def apply_to_ram(self) -> None:
        if not self._require_node():
            return
        self._expected_values = self._integer_values()
        self._start_command_sequence(self._configuration_commands() + ["CAL READ"])
        self.progress_label.setText("Applying calibration to node RAM...")

    def apply_and_save(self) -> None:
        if not self._require_node():
            return
        answer = QMessageBox.question(
            self,
            "Write EEPROM",
            "Apply these calibration values and store them in node EEPROM?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if answer != QMessageBox.StandardButton.Yes:
            return

        self._expected_values = self._integer_values()
        commands = self._configuration_commands() + ["CAL SAVE", "CAL READ"]
        self._start_command_sequence(commands)
        self.progress_label.setText("Applying calibration and writing EEPROM...")

    def confirm_reset(self) -> None:
        if not self._require_node():
            return
        answer = QMessageBox.question(
            self,
            "Factory reset calibration",
            "Erase the stored calibration and restore factory defaults?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if answer == QMessageBox.StandardButton.Yes:
            self._expected_values = None
            self._start_command_sequence(["CAL RESET", "CAL READ"])
            self.progress_label.setText("Resetting calibration...")

    def consume_calibration(self, message: CalibrationMessage) -> None:
        fields = message.fields
        try:
            self.state_label.setText(fields.get("STATE", "-"))
            self.version_label.setText(fields.get("VER", "-"))
            self.counter_label.setText(fields.get("COUNT", "-"))

            offsets = [int(fields[key]) / 100.0 for key in ("OX", "OY", "OZ")]
            gains = [int(fields[key]) / 1_000_000.0 for key in ("GX", "GY", "GZ")]
            noises = [int(fields[key]) / 10_000.0 for key in ("NX", "NY", "NZ")]
            temperature = int(fields["TCAL"]) / 100.0
        except (KeyError, TypeError, ValueError):
            self.progress_label.setText("Received an incomplete calibration record.")
            return

        for widget, value in zip(self.offset, offsets, strict=True):
            widget.setValue(value)
        for widget, value in zip(self.gain, gains, strict=True):
            widget.setValue(value)
        for widget, value in zip(self.noise, noises, strict=True):
            widget.setValue(value)
        self.temperature.setValue(temperature)

        if self._expected_values is None:
            self.progress_label.setText("Calibration record received.")
            return

        actual = {
            key: int(fields[key])
            for key in ("OX", "OY", "OZ", "GX", "GY", "GZ", "NX", "NY", "NZ", "TCAL")
        }
        mismatches = [
            key for key, expected in self._expected_values.items()
            if actual.get(key) != expected
        ]
        if mismatches:
            self.progress_label.setText(
                "Verification failed for: " + ", ".join(mismatches)
            )
        else:
            state = fields.get("STATE", "-")
            self.progress_label.setText(
                f"Calibration verified successfully; node state is {state}."
            )
        self._expected_values = None

    def consume_response(self, message: ResponseMessage) -> None:
        flags = message.flags
        fields = message.fields
        if "ERROR" in flags:
            reason = fields.get("REASON", "UNKNOWN")
            self.progress_label.setText(f"Node reported an error: {reason}")
        elif "OK" in flags:
            command = fields.get("CMD", "command")
            self.progress_label.setText(f"Node accepted {command}.")

    def _require_node(self) -> bool:
        node_id = self.selected_node()
        if node_id is None:
            QMessageBox.warning(self, "No node", "Select an active node first.")
            return False
        self.node_label.setText(str(node_id))
        return True

    def _integer_values(self) -> dict[str, int]:
        offsets = [round(widget.value() * 100.0) for widget in self.offset]
        gains = [round(widget.value() * 1_000_000.0) for widget in self.gain]
        noises = [round(widget.value() * 10_000.0) for widget in self.noise]
        temperature = round(self.temperature.value() * 100.0)
        return {
            "OX": offsets[0], "OY": offsets[1], "OZ": offsets[2],
            "GX": gains[0], "GY": gains[1], "GZ": gains[2],
            "NX": noises[0], "NY": noises[1], "NZ": noises[2],
            "TCAL": temperature,
        }

    def _configuration_commands(self) -> list[str]:
        values = self._integer_values()
        return [
            f"CAL OFFSET {values['OX']} {values['OY']} {values['OZ']}",
            f"CAL GAIN {values['GX']} {values['GY']} {values['GZ']}",
            f"CAL NOISE {values['NX']} {values['NY']} {values['NZ']}",
            f"CAL TEMP {values['TCAL']}",
        ]

    def _start_command_sequence(self, commands: list[str]) -> None:
        if self.command_timer.isActive() or self._command_queue:
            QMessageBox.information(
                self,
                "Operation active",
                "Wait for the current command sequence to finish.",
            )
            return
        self._command_queue = list(commands)
        self._send_next_queued_command()

    def _send_next_queued_command(self) -> None:
        if not self._command_queue:
            return
        command = self._command_queue.pop(0)
        if not self.send_command(command):
            self._command_queue.clear()
            self.progress_label.setText(f"Could not send: {command}")
            return
        if self._command_queue:
            # 250 ms is deliberately conservative at 4800 baud while the node
            # continues to publish measurement packets.
            self.command_timer.start(250)

from __future__ import annotations

import json
import statistics
from dataclasses import asdict, dataclass
from datetime import datetime
from pathlib import Path
from typing import Callable

from PyQt6.QtWidgets import (
    QDialog,
    QFileDialog,
    QFormLayout,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QMessageBox,
    QPushButton,
    QSpinBox,
    QVBoxLayout,
)

from .parser import ND280Packet


CAPTURE_NAMES = ("X+", "X-", "Y+", "Y-", "Z+", "Z-")


@dataclass
class AxisStatistics:
    mean_mt: float
    sigma_mt: float
    minimum_mt: float
    maximum_mt: float
    samples: int


@dataclass
class CalibrationResult:
    format_version: int
    created_iso: str
    board_id: int
    firmware: str
    method: str
    samples_per_position: int
    offset_x_mt: float
    offset_y_mt: float
    offset_z_mt: float
    gain_x: float
    gain_y: float
    gain_z: float
    reference_temperature_c: float
    noise_x_sigma_mt: float | None
    noise_y_sigma_mt: float | None
    noise_z_sigma_mt: float | None
    captures: dict[str, dict[str, float | int]]


class CalibrationDialog(QDialog):
    """Six-position offset calibration and stationary-noise acquisition.

    The six-position method does not establish an absolute gain calibration.
    For each sensor axis, the board must be positioned so that the same stable
    ambient field points first along +axis and then along -axis. The offset is
    estimated as (mean_plus + mean_minus) / 2.
    """

    def __init__(
        self,
        calibration_directory: Path,
        selected_node: Callable[[], int | None],
        parent=None,
    ) -> None:
        super().__init__(parent)
        self.setWindowTitle("ND280 six-position calibration")
        self.resize(720, 560)

        self.calibration_directory = calibration_directory
        self.selected_node = selected_node
        self.active_capture: str | None = None
        self.active_node_id: int | None = None
        self.capture_samples: list[ND280Packet] = []
        self.captures: dict[str, list[ND280Packet]] = {}
        self.noise_samples: list[ND280Packet] = []
        self.latest_result: CalibrationResult | None = None

        self._build_ui()

    def _build_ui(self) -> None:
        root = QVBoxLayout(self)

        instructions = QLabel(
            "Offset calibration without a reference gaussmeter. For each axis, "
            "place the same stable ambient field along +axis and then -axis. "
            "Keep the board fixed while each capture is running. Gain remains 1.0."
        )
        instructions.setWordWrap(True)
        root.addWidget(instructions)

        settings = QHBoxLayout()
        self.node_label = QLabel("Selected node: -")
        self.sample_spin = QSpinBox()
        self.sample_spin.setRange(20, 10000)
        self.sample_spin.setValue(500)
        self.sample_spin.setSingleStep(100)
        settings.addWidget(self.node_label)
        settings.addStretch()
        settings.addWidget(QLabel("Samples per capture"))
        settings.addWidget(self.sample_spin)
        root.addLayout(settings)

        capture_box = QGroupBox("Six-position captures")
        grid = QGridLayout(capture_box)
        self.capture_buttons: dict[str, QPushButton] = {}
        self.capture_labels: dict[str, QLabel] = {}

        for index, name in enumerate(CAPTURE_NAMES):
            row = index // 2
            col = (index % 2) * 2
            button = QPushButton(f"Capture {name}")
            label = QLabel("not acquired")
            button.clicked.connect(lambda _checked=False, n=name: self.start_capture(n))
            grid.addWidget(button, row, col)
            grid.addWidget(label, row, col + 1)
            self.capture_buttons[name] = button
            self.capture_labels[name] = label

        root.addWidget(capture_box)

        noise_box = QGroupBox("Stationary noise")
        noise_layout = QFormLayout(noise_box)
        self.noise_button = QPushButton("Acquire stationary noise")
        self.noise_label = QLabel("not acquired")
        self.noise_button.clicked.connect(self.start_noise_capture)
        noise_layout.addRow(self.noise_button, self.noise_label)
        root.addWidget(noise_box)

        result_box = QGroupBox("Calculated result")
        result_layout = QFormLayout(result_box)
        self.offset_x_label = QLabel("-")
        self.offset_y_label = QLabel("-")
        self.offset_z_label = QLabel("-")
        self.noise_x_label = QLabel("-")
        self.noise_y_label = QLabel("-")
        self.noise_z_label = QLabel("-")
        result_layout.addRow("Offset X", self.offset_x_label)
        result_layout.addRow("Offset Y", self.offset_y_label)
        result_layout.addRow("Offset Z", self.offset_z_label)
        result_layout.addRow("Noise sigma X", self.noise_x_label)
        result_layout.addRow("Noise sigma Y", self.noise_y_label)
        result_layout.addRow("Noise sigma Z", self.noise_z_label)
        root.addWidget(result_box)

        controls = QHBoxLayout()
        self.compute_button = QPushButton("Compute calibration")
        self.save_button = QPushButton("Save JSON")
        self.reset_button = QPushButton("Reset session")
        self.close_button = QPushButton("Close")
        self.save_button.setEnabled(False)
        self.compute_button.clicked.connect(self.compute_calibration)
        self.save_button.clicked.connect(self.save_result)
        self.reset_button.clicked.connect(self.reset_session)
        self.close_button.clicked.connect(self.close)
        controls.addWidget(self.compute_button)
        controls.addWidget(self.save_button)
        controls.addWidget(self.reset_button)
        controls.addStretch()
        controls.addWidget(self.close_button)
        root.addLayout(controls)

        self.progress_label = QLabel("Ready")
        root.addWidget(self.progress_label)

    def showEvent(self, event) -> None:  # noqa: N802
        node_id = self.selected_node()
        self.node_label.setText(f"Selected node: {node_id if node_id is not None else '-'}")
        super().showEvent(event)

    def start_capture(self, name: str) -> None:
        node_id = self.selected_node()
        if node_id is None:
            QMessageBox.warning(self, "No node", "Select an active node first.")
            return
        if self.active_capture is not None:
            QMessageBox.information(self, "Capture active", "Wait for the current capture to finish.")
            return

        self.active_capture = name
        self.active_node_id = node_id
        self.capture_samples = []
        self._set_capture_buttons_enabled(False)
        self.progress_label.setText(
            f"Capturing {name} for node {node_id}: 0/{self.sample_spin.value()}"
        )

    def start_noise_capture(self) -> None:
        node_id = self.selected_node()
        if node_id is None:
            QMessageBox.warning(self, "No node", "Select an active node first.")
            return
        if self.active_capture is not None:
            QMessageBox.information(self, "Capture active", "Wait for the current capture to finish.")
            return

        self.active_capture = "NOISE"
        self.active_node_id = node_id
        self.noise_samples = []
        self._set_capture_buttons_enabled(False)
        self.progress_label.setText(
            f"Capturing stationary noise for node {node_id}: 0/{self.sample_spin.value()}"
        )

    def consume_packet(self, packet: ND280Packet) -> None:
        if self.active_capture is None or packet.board_id != self.active_node_id:
            return

        target = self.sample_spin.value()
        if self.active_capture == "NOISE":
            self.noise_samples.append(packet)
            count = len(self.noise_samples)
            self.progress_label.setText(
                f"Capturing stationary noise for node {packet.board_id}: {count}/{target}"
            )
            if count >= target:
                self._finish_noise_capture()
        else:
            self.capture_samples.append(packet)
            count = len(self.capture_samples)
            self.progress_label.setText(
                f"Capturing {self.active_capture} for node {packet.board_id}: {count}/{target}"
            )
            if count >= target:
                self._finish_position_capture()

    def _finish_position_capture(self) -> None:
        assert self.active_capture is not None
        name = self.active_capture
        self.captures[name] = list(self.capture_samples)
        axis_values = self._axis_values(name, self.capture_samples)
        stats = self._stats(axis_values)
        self.capture_labels[name].setText(
            f"mean={stats.mean_mt:.5f} mT, sigma={stats.sigma_mt:.5f} mT, N={stats.samples}"
        )
        self.active_capture = None
        self.capture_samples = []
        self._set_capture_buttons_enabled(True)
        self.progress_label.setText(f"Capture {name} complete")

    def _finish_noise_capture(self) -> None:
        sx = self._stats([p.bx_mt for p in self.noise_samples])
        sy = self._stats([p.by_mt for p in self.noise_samples])
        sz = self._stats([p.bz_mt for p in self.noise_samples])
        self.noise_label.setText(
            f"sigma X/Y/Z = {sx.sigma_mt:.5f} / {sy.sigma_mt:.5f} / {sz.sigma_mt:.5f} mT"
        )
        self.noise_x_label.setText(f"{sx.sigma_mt:.6f} mT")
        self.noise_y_label.setText(f"{sy.sigma_mt:.6f} mT")
        self.noise_z_label.setText(f"{sz.sigma_mt:.6f} mT")
        self.active_capture = None
        self._set_capture_buttons_enabled(True)
        self.progress_label.setText("Stationary-noise capture complete")

    def compute_calibration(self) -> None:
        missing = [name for name in CAPTURE_NAMES if name not in self.captures]
        if missing:
            QMessageBox.warning(
                self,
                "Missing captures",
                "Acquire all six positions first: " + ", ".join(missing),
            )
            return

        x_plus = self._stats([p.bx_mt for p in self.captures["X+"]]).mean_mt
        x_minus = self._stats([p.bx_mt for p in self.captures["X-"]]).mean_mt
        y_plus = self._stats([p.by_mt for p in self.captures["Y+"]]).mean_mt
        y_minus = self._stats([p.by_mt for p in self.captures["Y-"]]).mean_mt
        z_plus = self._stats([p.bz_mt for p in self.captures["Z+"]]).mean_mt
        z_minus = self._stats([p.bz_mt for p in self.captures["Z-"]]).mean_mt

        offset_x = (x_plus + x_minus) / 2.0
        offset_y = (y_plus + y_minus) / 2.0
        offset_z = (z_plus + z_minus) / 2.0

        all_packets = [packet for values in self.captures.values() for packet in values]
        board_id = all_packets[0].board_id
        firmware = all_packets[0].firmware
        reference_temperature = statistics.fmean(p.temperature_c for p in all_packets)

        noise_x = noise_y = noise_z = None
        if len(self.noise_samples) >= 2:
            noise_x = self._stats([p.bx_mt for p in self.noise_samples]).sigma_mt
            noise_y = self._stats([p.by_mt for p in self.noise_samples]).sigma_mt
            noise_z = self._stats([p.bz_mt for p in self.noise_samples]).sigma_mt

        capture_summary: dict[str, dict[str, float | int]] = {}
        for name, packets in self.captures.items():
            stats = self._stats(self._axis_values(name, packets))
            capture_summary[name] = asdict(stats)

        self.latest_result = CalibrationResult(
            format_version=1,
            created_iso=datetime.now().isoformat(timespec="seconds"),
            board_id=board_id,
            firmware=firmware,
            method="six_position_ambient_field_offset",
            samples_per_position=self.sample_spin.value(),
            offset_x_mt=offset_x,
            offset_y_mt=offset_y,
            offset_z_mt=offset_z,
            gain_x=1.0,
            gain_y=1.0,
            gain_z=1.0,
            reference_temperature_c=reference_temperature,
            noise_x_sigma_mt=noise_x,
            noise_y_sigma_mt=noise_y,
            noise_z_sigma_mt=noise_z,
            captures=capture_summary,
        )

        self.offset_x_label.setText(f"{offset_x:.6f} mT")
        self.offset_y_label.setText(f"{offset_y:.6f} mT")
        self.offset_z_label.setText(f"{offset_z:.6f} mT")
        self.save_button.setEnabled(True)
        self.progress_label.setText("Calibration calculated. Gain remains 1.0 until reference calibration.")

    def save_result(self) -> None:
        if self.latest_result is None:
            return

        self.calibration_directory.mkdir(parents=True, exist_ok=True)
        default_name = (
            self.calibration_directory
            / f"node_{self.latest_result.board_id:02d}_calibration_{datetime.now():%Y-%m-%d_%H-%M-%S}.json"
        )
        path_string, _ = QFileDialog.getSaveFileName(
            self,
            "Save calibration",
            str(default_name),
            "JSON files (*.json)",
        )
        if not path_string:
            return

        path = Path(path_string)
        path.write_text(json.dumps(asdict(self.latest_result), indent=2), encoding="utf-8")
        self.progress_label.setText(f"Calibration saved to {path}")

    def reset_session(self) -> None:
        if self.active_capture is not None:
            QMessageBox.warning(self, "Capture active", "Wait for the current capture to finish.")
            return
        self.captures.clear()
        self.noise_samples.clear()
        self.latest_result = None
        for label in self.capture_labels.values():
            label.setText("not acquired")
        self.noise_label.setText("not acquired")
        for label in (
            self.offset_x_label,
            self.offset_y_label,
            self.offset_z_label,
            self.noise_x_label,
            self.noise_y_label,
            self.noise_z_label,
        ):
            label.setText("-")
        self.save_button.setEnabled(False)
        self.progress_label.setText("Session reset")

    def _set_capture_buttons_enabled(self, enabled: bool) -> None:
        for button in self.capture_buttons.values():
            button.setEnabled(enabled)
        self.noise_button.setEnabled(enabled)
        self.compute_button.setEnabled(enabled)
        self.reset_button.setEnabled(enabled)

    @staticmethod
    def _axis_values(name: str, packets: list[ND280Packet]) -> list[float]:
        axis = name[0]
        if axis == "X":
            return [p.bx_mt for p in packets]
        if axis == "Y":
            return [p.by_mt for p in packets]
        return [p.bz_mt for p in packets]

    @staticmethod
    def _stats(values: list[float]) -> AxisStatistics:
        if not values:
            raise ValueError("No samples")
        sigma = statistics.stdev(values) if len(values) >= 2 else 0.0
        return AxisStatistics(
            mean_mt=statistics.fmean(values),
            sigma_mt=sigma,
            minimum_mt=min(values),
            maximum_mt=max(values),
            samples=len(values),
        )

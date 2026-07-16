from __future__ import annotations

from PyQt6.QtWidgets import (
    QDialog,
    QFormLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
)

from .parser import DiagnosticMessage


def format_uptime(seconds: int) -> str:
    days, remainder = divmod(max(0, seconds), 86400)
    hours, remainder = divmod(remainder, 3600)
    minutes, secs = divmod(remainder, 60)
    if days:
        return f"{days} d {hours:02d} h {minutes:02d} min {secs:02d} s"
    if hours:
        return f"{hours} h {minutes:02d} min {secs:02d} s"
    if minutes:
        return f"{minutes} min {secs:02d} s"
    return f"{secs} s"


class DiagnosticsDashboard(QDialog):
    def __init__(self, send_command, parent=None) -> None:
        super().__init__(parent)
        self.send_command = send_command
        self.setWindowTitle("NodeOS Diagnostics Dashboard")
        self.resize(560, 520)
        self._last_fields: dict[str, str] = {}

        root = QVBoxLayout(self)

        controls = QHBoxLayout()
        self.refresh_button = QPushButton("Refresh DIAG")
        self.refresh_button.clicked.connect(lambda: self.send_command("DIAG"))
        self.wdt_test_button = QPushButton("Watchdog test")
        self.wdt_test_button.clicked.connect(self._request_watchdog_test)
        controls.addWidget(self.refresh_button)
        controls.addWidget(self.wdt_test_button)
        controls.addStretch()
        root.addLayout(controls)

        health_box = QGroupBox("Node health")
        health_layout = QFormLayout(health_box)
        self.health_label = QLabel("WAITING")
        self.health_label.setStyleSheet("font-weight: bold; color: #b36b00;")
        self.node_label = QLabel("-")
        self.uptime_label = QLabel("-")
        self.reset_label = QLabel("-")
        self.reset_flags_label = QLabel("-")
        health_layout.addRow("Health", self.health_label)
        health_layout.addRow("Node ID", self.node_label)
        health_layout.addRow("Uptime", self.uptime_label)
        health_layout.addRow("Last reset", self.reset_label)
        health_layout.addRow("Reset flags", self.reset_flags_label)
        root.addWidget(health_box)

        counters_box = QGroupBox("Persistent and runtime counters")
        counters = QFormLayout(counters_box)
        self.labels: dict[str, QLabel] = {}
        rows = (
            ("BOOTS", "Boot count"),
            ("WDT_RESETS", "Watchdog resets"),
            ("MEAS_OK", "Measurements OK"),
            ("MEAS_FAIL", "Measurements failed"),
            ("TWI_ERR", "TWI errors"),
            ("UART_CMD", "UART commands"),
            ("UART_OVF", "UART overflows"),
            ("EEPROM_ERR", "EEPROM errors"),
        )
        for key, caption in rows:
            label = QLabel("-")
            self.labels[key] = label
            counters.addRow(caption, label)
        root.addWidget(counters_box)

        self.raw_label = QLabel("No diagnostic packet received.")
        self.raw_label.setWordWrap(True)
        root.addWidget(self.raw_label)


    def showEvent(self, event) -> None:  # noqa: N802
        super().showEvent(event)
        # Request one diagnostic snapshot when the window opens.
        # Continuous polling is intentionally disabled because some USB-UART
        # bridge configurations can duplicate or queue commands, potentially
        # starving the node main loop and provoking a watchdog reset.
        self.send_command("DIAG")

    def closeEvent(self, event) -> None:  # noqa: N802
        event.accept()

    def _request_watchdog_test(self) -> None:
        from PyQt6.QtWidgets import QMessageBox
        answer = QMessageBox.warning(
            self,
            "Watchdog reset test",
            "This command intentionally stops servicing the watchdog and resets the node.\n\nContinue?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if answer == QMessageBox.StandardButton.Yes:
            self.send_command("DIAG WDT TEST")

    def consume_diagnostic(self, message: DiagnosticMessage) -> None:
        fields = message.fields
        self._last_fields = dict(fields)
        self.node_label.setText(fields.get("ID", "-"))

        try:
            uptime = int(fields.get("UPTIME_S", "0"))
            self.uptime_label.setText(format_uptime(uptime))
        except ValueError:
            self.uptime_label.setText(fields.get("UPTIME_S", "-"))

        self.reset_label.setText(fields.get("RESET", "-"))
        self.reset_flags_label.setText(fields.get("RESET_FLAGS", "-"))
        for key, label in self.labels.items():
            label.setText(fields.get(key, "-"))

        error_keys = ("MEAS_FAIL", "TWI_ERR", "UART_OVF", "EEPROM_ERR")
        errors = 0
        for key in error_keys:
            try:
                errors += int(fields.get(key, "0"))
            except ValueError:
                errors += 1

        if errors == 0:
            self.health_label.setText("OK")
            self.health_label.setStyleSheet("font-weight: bold; color: #17823b;")
        else:
            self.health_label.setText("ATTENTION")
            self.health_label.setStyleSheet("font-weight: bold; color: #b00020;")

        self.raw_label.setText(message.raw_line)

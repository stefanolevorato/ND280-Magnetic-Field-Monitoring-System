from __future__ import annotations

import csv
from datetime import datetime
from pathlib import Path
from time import monotonic

from .parser import ND280Packet


class CSVLogger:
    def __init__(self, log_directory: Path) -> None:
        self.log_directory = log_directory
        self.file = None
        self.writer = None
        self.start_time = 0.0
        self.path: Path | None = None

    @property
    def active(self) -> bool:
        return self.file is not None

    def start(self) -> Path:
        self.log_directory.mkdir(parents=True, exist_ok=True)
        stamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
        self.path = self.log_directory / f"nd280_{stamp}.csv"
        self.file = self.path.open("w", newline="", encoding="utf-8")
        self.writer = csv.writer(self.file)
        self.writer.writerow([
            "host_time_iso",
            "elapsed_s",
            "firmware",
            "board_id",
            "sequence",
            "average",
            "temperature_c",
            "bx_mt",
            "by_mt",
            "bz_mt",
            "status_hex",
            "raw_line",
        ])
        self.start_time = monotonic()
        return self.path

    def write(self, packet: ND280Packet) -> None:
        if self.writer is None or self.file is None:
            return
        self.writer.writerow([
            datetime.now().isoformat(timespec="milliseconds"),
            f"{monotonic() - self.start_time:.3f}",
            packet.firmware,
            packet.board_id,
            packet.sequence,
            packet.average,
            f"{packet.temperature_c:.4f}",
            f"{packet.bx_mt:.4f}",
            f"{packet.by_mt:.4f}",
            f"{packet.bz_mt:.4f}",
            f"0x{packet.status:02X}",
            packet.raw_line,
        ])
        self.file.flush()

    def stop(self) -> None:
        if self.file is not None:
            self.file.close()
        self.file = None
        self.writer = None

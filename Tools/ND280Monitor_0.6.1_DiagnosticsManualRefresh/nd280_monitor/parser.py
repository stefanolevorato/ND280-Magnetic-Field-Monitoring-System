from __future__ import annotations

from dataclasses import dataclass
from typing import TypeAlias


@dataclass(slots=True)
class ND280Packet:
    firmware: str
    board_id: int
    sequence: int
    average: int
    temperature_c: float
    bx_mt: float
    by_mt: float
    bz_mt: float
    status: int
    raw_line: str


@dataclass(slots=True)
class InfoMessage:
    fields: dict[str, str]
    raw_line: str


@dataclass(slots=True)
class CalibrationMessage:
    fields: dict[str, str]
    raw_line: str


@dataclass(slots=True)
class ResponseMessage:
    fields: dict[str, str]
    flags: set[str]
    raw_line: str


@dataclass(slots=True)
class DiagnosticMessage:
    fields: dict[str, str]
    raw_line: str


ProtocolMessage: TypeAlias = (
    ND280Packet
    | InfoMessage
    | CalibrationMessage
    | ResponseMessage
    | DiagnosticMessage
)


def _parse_tokens(line: str) -> tuple[dict[str, str], set[str]]:
    fields: dict[str, str] = {}
    flags: set[str] = set()
    for token in line.split(",")[1:]:
        token = token.strip()
        if not token:
            continue
        if "=" in token:
            key, value = token.split("=", 1)
            fields[key.strip().upper()] = value.strip()
        else:
            flags.add(token.upper())
    return fields, flags


def parse_packet(line: str) -> ND280Packet | None:
    line = line.strip()
    if not line.startswith("$ND280,"):
        return None

    fields, _flags = _parse_tokens(line)
    required = {"VER", "ID", "SEQ", "T", "BX", "BY", "BZ", "STATUS"}
    if not required.issubset(fields):
        return None

    try:
        return ND280Packet(
            firmware=fields["VER"],
            board_id=int(fields["ID"], 10),
            sequence=int(fields["SEQ"], 10),
            average=int(fields.get("AVG", "1"), 10),
            temperature_c=float(fields["T"]),
            bx_mt=float(fields["BX"]),
            by_mt=float(fields["BY"]),
            bz_mt=float(fields["BZ"]),
            status=int(fields["STATUS"], 0),
            raw_line=line,
        )
    except (ValueError, TypeError):
        return None


def parse_message(line: str) -> ProtocolMessage | None:
    line = line.strip()

    packet = parse_packet(line)
    if packet is not None:
        return packet

    fields, flags = _parse_tokens(line)

    if line.startswith("$ND280INFO,"):
        return InfoMessage(fields, line)

    if line.startswith("$ND280CAL,"):
        return CalibrationMessage(fields, line)

    if line.startswith("$ND280RSP,"):
        return ResponseMessage(fields, flags, line)

    if line.startswith("$ND280DIAG,"):
        return DiagnosticMessage(fields, line)

    return None

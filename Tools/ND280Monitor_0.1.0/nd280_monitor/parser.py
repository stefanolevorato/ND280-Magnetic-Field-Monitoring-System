from __future__ import annotations

from dataclasses import dataclass


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


def parse_packet(line: str) -> ND280Packet | None:
    line = line.strip()
    if not line.startswith("$ND280,"):
        return None

    fields: dict[str, str] = {}
    for token in line.split(",")[1:]:
        if "=" not in token:
            return None
        key, value = token.split("=", 1)
        fields[key.strip().upper()] = value.strip()

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

from __future__ import annotations

COMMAND_PREFIX = ":"
LINE_TERMINATOR = "\r\n"


def frame_command(command: str) -> bytes:
    """Return one NodeOS command frame ready for serial transmission.

    The GUI and all dialogs use unframed commands such as ``DIAG`` or
    ``CAL READ``. NodeOS v0.5.3 and later require a leading colon. This
    function is the only place where that protocol detail is applied.
    """
    normalized = command.strip()
    if not normalized:
        raise ValueError("Command is empty")

    if not normalized.startswith(COMMAND_PREFIX):
        normalized = COMMAND_PREFIX + normalized

    return (normalized + LINE_TERMINATOR).encode("ascii", errors="strict")

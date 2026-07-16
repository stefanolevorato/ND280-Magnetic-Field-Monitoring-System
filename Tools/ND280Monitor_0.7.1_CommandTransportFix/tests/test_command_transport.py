from nd280_monitor.command_transport import frame_command


def main() -> None:
    assert frame_command("DIAG") == b":DIAG\r\n"
    assert frame_command("  CAL READ  ") == b":CAL READ\r\n"
    assert frame_command(":HELP") == b":HELP\r\n"
    try:
        frame_command("   ")
    except ValueError:
        pass
    else:
        raise AssertionError("Empty command must be rejected")


if __name__ == "__main__":
    main()
    print("command transport tests: PASS")

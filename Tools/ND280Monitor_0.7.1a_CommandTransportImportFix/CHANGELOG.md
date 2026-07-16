# Changelog

## 0.7.1a

### Fixed

- Imported `frame_command` in `serial_worker.py`.
- Restored command transmission from all GUI buttons and dialogs.
- Eliminated the misleading secondary "serial port is not ready" warning caused by the missing symbol.

## 0.7.1a

### Fixed

- Centralized NodeOS command framing in `command_transport.py`.
- Ensured every GUI button and dialog command receives the mandatory `:` prefix.
- Prevented duplicate prefixes when the user enters `:` manually.
- Retained the Clear Serial Console control.
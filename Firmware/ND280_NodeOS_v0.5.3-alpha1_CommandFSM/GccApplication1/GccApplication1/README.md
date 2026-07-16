# NodeOS v0.5.3-alpha1 - Deterministic UART Command FSM

This release starts from the validated `v0.5.2-alpha1a` baseline and replaces
only the UART command receiver with a bounded finite-state machine.

## Command syntax

Every command must begin with `:` and end with CR, LF, or CRLF.

Examples:

```text
:HELP
:INFO
:DIAG
:CAL READ
:DIAG WDT TEST
```

Bytes received outside a prefixed command are ignored. This includes noise,
startup fragments and any echoed `$ND280...` output.

## Important compatibility note

The current Windows monitor must be updated to prepend `:` to outgoing
commands. Until that update, use Thonny or another terminal and send the
prefixed commands manually.

## Microchip Studio

Use `MicrochipStudio_Flat` and add the two new files:

```text
command_parser.c
command_parser.h
```

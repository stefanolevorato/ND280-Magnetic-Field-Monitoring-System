ND280 Magnetic Field Node v0.3.1 - NodeOS command protocol
==========================================================

Target:
  ATmega328PB, internal 8 MHz RC with CKDIV8 -> F_CPU = 1 MHz
  UART0 = 4800 baud, 8N1
  TMAG5273A2 on TWI1, range +/-266 mT

New in v0.3.1:
  - UART0 receive enabled using an interrupt-driven 128-byte ring buffer.
  - Bidirectional ASCII command protocol.
  - Calibration can be edited in RAM, saved to EEPROM, read back, or reset.
  - CAL=DIRTY is transmitted while RAM values differ from saved EEPROM values.

Pico wiring for bidirectional operation:
  Node PD1 / TXD0 (pin 31) -> Pico GP1 / UART0 RX
  Node PD0 / RXD0 (pin 30) <- Pico GP0 / UART0 TX
  Node GND                  -> Pico GND

Both devices use 3.3 V logic. Do not connect Pico 3V3 if the node has its own supply.

Commands (terminate with CR or LF):
  HELP
  INFO
  CAL READ
  CAL OFFSET x y z
  CAL GAIN x y z
  CAL NOISE x y z
  CAL TEMP t
  CAL SAVE
  CAL RESET

Units:
  CAL OFFSET: hundredths of mT. Example -20 means -0.20 mT.
  CAL GAIN: parts per million. 1000000 means gain 1.000000.
  CAL NOISE: 0.0001 mT. Example 449 means 0.0449 mT.
  CAL TEMP: hundredths of degC. Example 2987 means 29.87 C.

Example session:
  INFO
  CAL READ
  CAL OFFSET -20 -24 -21
  CAL NOISE 449 461 318
  CAL TEMP 2987
  CAL SAVE
  CAL READ

CAL SAVE increments the calibration counter and verifies the EEPROM write.
CAL RESET erases the EEPROM record and restores safe defaults in RAM.

Important:
  Board ID always comes from the hardware DIP switches and is never stored in EEPROM.

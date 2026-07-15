ND280 Magnetic Field Node v0.3.0 - EEPROM calibration framework
================================================================

Target:
  ATmega328PB
  F_CPU = 1 MHz
  TMAG5273A2, +/-266 mT
  UART0 = 4800 baud
  TWI1 = PE0/PE1

New in v0.3.0:
  - Versioned EEPROM calibration record
  - CRC-16/CCITT integrity check
  - Default fallback when EEPROM is blank or invalid
  - Calibration offsets and gains applied to Bx/By/Bz
  - Calibration status included in each serial packet
  - EEPROM save/load/factory-reset API ready for the next command protocol step

Files to add to a Microchip Studio GCC C project:
  main.c
  config.h
  uart0.c / uart0.h
  twi1.c / twi1.h
  board_id.c / board_id.h
  tmag5273.c / tmag5273.h
  calibration.c / calibration.h
  crc16.c / crc16.h

First boot with blank EEPROM:
  CALIBRATION=DEFAULT
  offsets = 0.00 mT
  gains = 1.000000

Packets remain compatible with the current monitor. An additional field is present:
  CAL=DEFAULT
or
  CAL=VALID

Example:
  $ND280,VER=0.3.0,ID=4,SEQ=0,AVG=10,CAL=DEFAULT,T=29.87,BX=-0.20,BY=-0.24,BZ=-0.21,STATUS=0xF1

Important:
  This release defines and validates EEPROM storage but does not yet expose
  commands for writing calibration values from the PC. That is the next step.

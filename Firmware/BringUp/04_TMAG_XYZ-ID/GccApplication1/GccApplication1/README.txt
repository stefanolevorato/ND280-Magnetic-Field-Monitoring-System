BU008 - Board ID + TMAG Data

Target: ATmega328PB, F_CPU=1 MHz, UART0=4800 baud, TWI1=PE0/PE1.

Files to add to Microchip Studio project:
  main.c
  uart0.c
  uart0.h
  twi1.c
  twi1.h

DIP mapping:
  bit0 = PC0, physical pin 23
  bit1 = PC1, physical pin 24
  bit2 = PC2, physical pin 25
  bit3 = PC3, physical pin 26
  bit4 = PD2, physical pin 32

Electrical logic:
  External 100 kOhm pull-ups to 3.3 V.
  Open switch = 1.
  ON/closed to GND = 0.
  BOARD_ID = (~RAW_DIP) & 0x1F.

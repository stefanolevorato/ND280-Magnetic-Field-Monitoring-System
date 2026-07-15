ND280 Magnetic Field Node v0.1.0
================================

Target:
  ATmega328PB
  F_CPU = 1 MHz (factory CKDIV8 still enabled)
  UART0 = 4800 baud
  TWI1 = PE0/SDA1 and PE1/SCL1
  TMAG5273 address = 0x35

Files to add to a new Microchip Studio GCC C Executable Project:
  main.c
  uart0.c
  uart0.h
  twi1.c
  twi1.h
  board_id.c
  board_id.h

Important:
  uart0.* and twi1.* are copied unchanged from the known-good BU007 project.
  The TMAG initialization and read sequence in main.c are also copied from BU007.
  Only the proven Board ID module and the new serial packet format were added.

Board ID DIP mapping:
  bit 0 = PC0, physical pin 23
  bit 1 = PC1, physical pin 24
  bit 2 = PC2, physical pin 25
  bit 3 = PC3, physical pin 26
  bit 4 = PD2, physical pin 32

Electrical logic:
  External 100 kohm pull-ups to 3.3 V.
  Open switch = HIGH.
  ON/closed switch = LOW.
  ID = inverted 5-bit raw state, range 0 to 31.

Example packet:
  $ND280,VER=0.1.0,ID=3,T=32.85,BX=0.01,BY=-0.03,BZ=0.00,STATUS=0x73

The packet intentionally has no checksum yet. A CRC can be added after this
integration version is confirmed stable.

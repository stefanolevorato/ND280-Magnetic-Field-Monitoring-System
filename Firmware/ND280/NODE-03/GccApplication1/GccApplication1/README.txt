ND280 Magnetic Field Node firmware v0.2.1
==========================================

Target
------
ATmega328PB, factory clock fuses, effective CPU clock 1 MHz.
UART0: 4800 baud, 8N1.
TWI1: PE0/SDA1 and PE1/SCL1.
TMAG5273 I2C address: 0x35.
Populated sensor: TMAG5273A2.
Configured range: +/-266 mT.

New in v0.2.1
-------------
- Correct field scaling for the TMAG5273A2 in the 266 mT range.
- Magnetic conversion is now controlled from config.h.
- Startup output reports sensor variant and configured field range.
- Covers the required field measurements up to 200 mT with margin.

Files to add to a Microchip Studio GCC C project
-------------------------------------------------
main.c
config.h
uart0.c
uart0.h
twi1.c
twi1.h
board_id.c
board_id.h
tmag5273.c
tmag5273.h

Expected startup output
-----------------------
ND280 Magnetic Field Node v0.2.1
UART0=4800,TWI1=PE0/PE1
AVERAGE_SAMPLES=10,SAMPLE_INTERVAL_MS=100
TMAG_VARIANT=A2,TMAG_RANGE_MT=266
TMAG_DEVICE_ID=0x06
TMAG_INIT=OK
BOARD_ID=31

Expected data packet
--------------------
$ND280,VER=0.2.1,ID=31,SEQ=0,AVG=10,T=30.25,BX=-0.13,BY=-0.27,BZ=-0.20,STATUS=0x13

Important
---------
The firmware writes SENSOR_CONFIG_2 = 0x03, selecting the 2x range.
For TMAG5273A2 this corresponds to +/-266 mT full scale.

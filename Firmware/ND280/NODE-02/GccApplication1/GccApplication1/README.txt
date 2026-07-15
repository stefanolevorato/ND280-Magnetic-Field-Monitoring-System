ND280 Magnetic Field Node firmware v0.2.0
==========================================

Target
------
ATmega328PB, factory clock fuses, effective CPU clock 1 MHz.
UART0: 4800 baud, 8N1.
TWI1: PE0/SDA1 and PE1/SCL1.
TMAG5273 I2C address: 0x35.

New in v0.2.0
-------------
- 10-sample averaging performed on raw TMAG values.
- Samples are spaced by 100 ms.
- One output packet is produced approximately once per second.
- 32-bit packet sequence counter.
- AVG field reports the number of valid samples used.
- TMAG handling moved to tmag5273.c/.h.
- Acquisition settings moved to config.h.

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

Expected output
---------------
ND280 Magnetic Field Node v0.2.0
UART0=4800,TWI1=PE0/PE1
AVERAGE_SAMPLES=10,SAMPLE_INTERVAL_MS=100
TMAG_DEVICE_ID=0x06
TMAG_INIT=OK
BOARD_ID=31
$ND280,VER=0.2.0,ID=31,SEQ=0,AVG=10,T=30.25,BX=-0.02,BY=-0.04,BZ=-0.03,STATUS=0x13

Notes
-----
The known-good TMAG configuration and standby-trigger sequence from v0.1.0
have been preserved. The averaging is done on signed raw ADC values before
conversion to temperature and millitesla.

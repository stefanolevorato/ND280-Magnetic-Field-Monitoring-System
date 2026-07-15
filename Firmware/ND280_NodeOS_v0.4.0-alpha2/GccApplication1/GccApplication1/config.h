#ifndef CONFIG_H
#define CONFIG_H

#define F_CPU 1000000UL

#define FIRMWARE_VERSION "0.4.0-alpha2"
#define TMAG_I2C_ADDRESS 0x35

/* Populated sensor: TMAG5273A2. */
#define TMAG_VARIANT "A2"
#define TMAG_FULL_SCALE_MT 266L
#define TMAG_FULL_SCALE_X100 (TMAG_FULL_SCALE_MT * 100L)

/* Acquisition settings: 10 samples spaced by 100 ms -> about one packet/s. */
#define AVERAGE_SAMPLES 10U
#define SAMPLE_INTERVAL_MS 100U

/* UART command receiver. Must be a power of two. */
#define UART0_RX_BUFFER_SIZE 128U
#define UART0_COMMAND_MAX_LENGTH 96U

#endif

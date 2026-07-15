#ifndef CONFIG_H
#define CONFIG_H

#define F_CPU 1000000UL

/* -------------------------------------------------------------------------
 * NodeOS identity
 * ------------------------------------------------------------------------- */
#define NODEOS_VERSION          "0.5.2-alpha2"
#define FIRMWARE_VERSION        NODEOS_VERSION
#define NODEOS_PROTOCOL_VERSION 1U
#define NODE_BOARD_REVISION     "A"
#define NODE_HARDWARE_NAME      "TMAG Debug Board"
#define NODE_MCU_NAME           "ATmega328PB"
#define NODE_SENSOR_NAME        "TMAG5273A2"

/* -------------------------------------------------------------------------
 * Sensor
 * ------------------------------------------------------------------------- */
#define TMAG_I2C_ADDRESS 0x35
#define TMAG_VARIANT "A2"
#define TMAG_FULL_SCALE_MT 266L
#define TMAG_FULL_SCALE_X100 (TMAG_FULL_SCALE_MT * 100L)

/* -------------------------------------------------------------------------
 * Acquisition
 * ------------------------------------------------------------------------- */
#define AVERAGE_SAMPLES 10U
#define SAMPLE_INTERVAL_MS 100U

/* -------------------------------------------------------------------------
 * UART command receiver. Buffer size must be a power of two.
 * ------------------------------------------------------------------------- */
#define UART0_RX_BUFFER_SIZE 128U
#define UART0_COMMAND_MAX_LENGTH 96U

/* -------------------------------------------------------------------------
 * LED power-on self test. Values visually validated on Rev.A.
 * ------------------------------------------------------------------------- */
#define POST_SINGLE_LED_MS 240U
#define POST_ALL_LEDS_MS   500U
#define POST_GAP_MS        120U

/* -------------------------------------------------------------------------
 * Diagnostics timer: Timer0 CTC, 1 ms tick at F_CPU=1 MHz and prescaler 8.
 * ------------------------------------------------------------------------- */
#define DIAGNOSTICS_TIMER0_COMPARE 124U

#endif

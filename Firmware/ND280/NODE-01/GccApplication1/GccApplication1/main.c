#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "uart0.h"
#include "twi1.h"
#include "board_id.h"

#define FIRMWARE_VERSION "0.1.0"
#define TMAG_ADDR 0x35

#define DEVICE_CONFIG_1 0x00
#define DEVICE_CONFIG_2 0x01
#define SENSOR_CONFIG_1 0x02
#define SENSOR_CONFIG_2 0x03
#define T_CONFIG         0x07
#define DEVICE_ID        0x0D
#define T_MSB_RESULT     0x10

#define TSENSET0_X100 2500L
#define TADCT0        17508L
#define TADCRES_X10   601L

static int16_t make_i16(uint8_t msb, uint8_t lsb)
{
    return (int16_t)(((uint16_t)msb << 8) | lsb);
}

static uint8_t tmag_init_basic(void)
{
    /* This is the same known-good initialization used by BU007. */
    if (!twi1_write_register(TMAG_ADDR, DEVICE_CONFIG_1, 0x00)) return 0;
    if (!twi1_write_register(TMAG_ADDR, DEVICE_CONFIG_2, 0x00)) return 0;
    if (!twi1_write_register(TMAG_ADDR, SENSOR_CONFIG_1, 0x74)) return 0;
    if (!twi1_write_register(TMAG_ADDR, SENSOR_CONFIG_2, 0x03)) return 0;
    if (!twi1_write_register(TMAG_ADDR, T_CONFIG, 0x01)) return 0;
    return 1;
}

static uint8_t tmag_trigger_and_read(uint8_t *buf9)
{
    /* Same known-good standby trigger and 9-byte read used by BU007. */
    if (!twi1_write_register(TMAG_ADDR, (uint8_t)(DEVICE_ID | 0x80), 0x00)) return 0;
    _delay_ms(5);
    return twi1_read_block(TMAG_ADDR, T_MSB_RESULT, buf9, 9);
}

static void print_packet(uint8_t node_id,
                         int32_t temp_x100,
                         int32_t bx_x100,
                         int32_t by_x100,
                         int32_t bz_x100,
                         uint8_t status)
{
    /* Machine-readable, one complete sample per line. */
    uart0_print("$ND280,VER=");
    uart0_print(FIRMWARE_VERSION);
    uart0_print(",ID=");
    uart0_print_uint16(node_id);
    uart0_print(",T=");
    uart0_print_fixed_2(temp_x100);
    uart0_print(",BX=");
    uart0_print_fixed_2(bx_x100);
    uart0_print(",BY=");
    uart0_print_fixed_2(by_x100);
    uart0_print(",BZ=");
    uart0_print_fixed_2(bz_x100);
    uart0_print(",STATUS=0x");
    uart0_print_hex8(status);
    uart0_print("\r\n");
}

int main(void)
{
    uart0_init();
    twi1_init();
    board_id_init();

    DDRD |= (1 << PD5) | (1 << PD6) | (1 << PD7);

    uart0_print("\r\nND280 Magnetic Field Node v");
    uart0_print(FIRMWARE_VERSION);
    uart0_print("\r\nUART0=4800,TWI1=PE0/PE1\r\n");

    uint8_t device_id = 0;
    if (!twi1_read_register(TMAG_ADDR, DEVICE_ID, &device_id)) {
        uart0_print("ERROR,TMAG_DEVICE_ID_READ\r\n");
        while (1) {
            PORTD ^= (1 << PD7);
            _delay_ms(200);
        }
    }

    uart0_print("TMAG_DEVICE_ID=0x");
    uart0_print_hex8(device_id);
    uart0_print("\r\n");

    if (!tmag_init_basic()) {
        uart0_print("ERROR,TMAG_INIT\r\n");
        while (1) {
            PORTD ^= (1 << PD7);
            _delay_ms(200);
        }
    }

    uart0_print("TMAG_INIT=OK\r\n");
    uart0_print("BOARD_ID=");
    uart0_print_uint16(board_id_read());
    uart0_print("\r\n");

    while (1)
    {
        uint8_t b[9];

        if (!tmag_trigger_and_read(b)) {
            uart0_print("ERROR,TMAG_DATA_READ\r\n");
            PORTD ^= (1 << PD7);
            _delay_ms(1000);
            continue;
        }

        int16_t temp_raw = make_i16(b[0], b[1]);
        int16_t x_raw    = make_i16(b[2], b[3]);
        int16_t y_raw    = make_i16(b[4], b[5]);
        int16_t z_raw    = make_i16(b[6], b[7]);
        uint8_t status   = b[8];

        int32_t temp_x100 = TSENSET0_X100
                          + (((int32_t)temp_raw - TADCT0) * 1000L) / TADCRES_X10;

        /* 40 mT range, values expressed in hundredths of mT. */
        int32_t bx_x100 = ((int32_t)x_raw * 4000L) / 32768L;
        int32_t by_x100 = ((int32_t)y_raw * 4000L) / 32768L;
        int32_t bz_x100 = ((int32_t)z_raw * 4000L) / 32768L;

        print_packet(board_id_read(), temp_x100, bx_x100, by_x100, bz_x100, status);

        PORTD ^= (1 << PD5);
        _delay_ms(1000);
    }
}

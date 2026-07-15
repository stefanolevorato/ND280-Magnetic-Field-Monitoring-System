#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "uart0.h"
#include "twi1.h"

#define TMAG_ADDR 0x35

#define DEVICE_CONFIG_1 0x00
#define DEVICE_CONFIG_2 0x01
#define SENSOR_CONFIG_1 0x02
#define SENSOR_CONFIG_2 0x03
#define T_CONFIG        0x07
#define DEVICE_ID       0x0D
#define T_MSB_RESULT    0x10

#define TSENSET0_X100 2500L
#define TADCT0        17508L
#define TADCRES_X10   601L   // 60.1 LSB/degC, scaled by 10

static int16_t make_i16(uint8_t msb, uint8_t lsb)
{
    return (int16_t)(((uint16_t)msb << 8) | lsb);
}

static uint8_t tmag_init_basic(void)
{
    // Same conservative defaults used by the uploaded Arduino library,
    // but with temperature channel enabled for this test.
    if (!twi1_write_register(TMAG_ADDR, DEVICE_CONFIG_1, 0x00)) return 0; // standby, standard read mode, 1x average
    if (!twi1_write_register(TMAG_ADDR, DEVICE_CONFIG_2, 0x00)) return 0; // standby mode
    if (!twi1_write_register(TMAG_ADDR, SENSOR_CONFIG_1, 0x74)) return 0; // enable XYZ magnetic channels
    if (!twi1_write_register(TMAG_ADDR, SENSOR_CONFIG_2, 0x03)) return 0; // 40 mT range multiple setting from library defaults
    if (!twi1_write_register(TMAG_ADDR, T_CONFIG,        0x01)) return 0; // enable temperature channel
    return 1;
}

static uint8_t tmag_trigger_and_read(uint8_t *buf9)
{
    // Trigger conversion in standby mode. This follows the uploaded Arduino driver
    // technique: write to a read-only register address with bit 7 set.
    if (!twi1_write_register(TMAG_ADDR, (uint8_t)(DEVICE_ID | 0x80), 0x00)) return 0;

    // Conservative delay: enough for temp + X/Y/Z at 1x averaging.
    _delay_ms(5);

    return twi1_read_block(TMAG_ADDR, T_MSB_RESULT, buf9, 9);
}

int main(void)
{
    uart0_init();
    twi1_init();

    DDRD |= (1 << PD5) | (1 << PD6) | (1 << PD7);

    uart0_print("\r\nBU007 - TMAG5273 First Data\r\n");
    uart0_print("UART0: 4800 baud\r\n");
    uart0_print("TWI1 : PE0=SDA1, PE1=SCL1\r\n\r\n");

    uint8_t id = 0;
    if (!twi1_read_register(TMAG_ADDR, DEVICE_ID, &id)) {
        uart0_print("ERROR: cannot read DEVICE_ID\r\n");
        while (1) { PORTD ^= (1 << PD7); _delay_ms(200); }
    }

    uart0_print("DEVICE_ID = 0x");
    uart0_print_hex8(id);
    uart0_print("\r\n");

    if (!tmag_init_basic()) {
        uart0_print("ERROR: TMAG init failed\r\n");
        while (1) { PORTD ^= (1 << PD7); _delay_ms(200); }
    }

    uart0_print("TMAG init: OK\r\n\r\n");

    while (1)
    {
        uint8_t b[9];
        if (!tmag_trigger_and_read(b)) {
            uart0_print("ERROR: data read failed\r\n");
            PORTD ^= (1 << PD7);
            _delay_ms(1000);
            continue;
        }

        int16_t temp_raw = make_i16(b[0], b[1]);
        int16_t x_raw    = make_i16(b[2], b[3]);
        int16_t y_raw    = make_i16(b[4], b[5]);
        int16_t z_raw    = make_i16(b[6], b[7]);
        uint8_t status   = b[8];

        // Temperature: T = 25 + (TADCT - 17508) / 60.1
        int32_t temp_x100 = TSENSET0_X100 + (((int32_t)temp_raw - TADCT0) * 1000L) / TADCRES_X10;

        // Magnetic field in mT, assuming 40 mT range:
        // B = 40 * raw / 32768. Scaled by 100 for printing.
        int32_t bx_x100 = ((int32_t)x_raw * 4000L) / 32768L;
        int32_t by_x100 = ((int32_t)y_raw * 4000L) / 32768L;
        int32_t bz_x100 = ((int32_t)z_raw * 4000L) / 32768L;

        uart0_print("TEMP = ");
        uart0_print_fixed_2(temp_x100);
        uart0_print(" C, ");

        uart0_print("BX = ");
        uart0_print_fixed_2(bx_x100);
        uart0_print(" mT, BY = ");
        uart0_print_fixed_2(by_x100);
        uart0_print(" mT, BZ = ");
        uart0_print_fixed_2(bz_x100);
        uart0_print(" mT, STATUS = 0x");
        uart0_print_hex8(status);
        uart0_print("\r\n");

        PORTD ^= (1 << PD5);
        _delay_ms(1000);
    }
}

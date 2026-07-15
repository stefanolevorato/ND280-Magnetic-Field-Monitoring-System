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

static void board_id_init(void)
{
    DDRC &= ~((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3));
    DDRD &= ~(1 << PD2);
    PORTC &= ~((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3));
    PORTD &= ~(1 << PD2);
}

static uint8_t board_id_read_raw(void)
{
    uint8_t raw = 0;
    if (PINC & (1 << PC0)) raw |= (1 << 0);
    if (PINC & (1 << PC1)) raw |= (1 << 1);
    if (PINC & (1 << PC2)) raw |= (1 << 2);
    if (PINC & (1 << PC3)) raw |= (1 << 3);
    if (PIND & (1 << PD2)) raw |= (1 << 4);
    return raw;
}

static uint8_t board_id_read(void)
{
    return (~board_id_read_raw()) & 0x1F;
}

static void uart0_print_bin5(uint8_t v)
{
    for (int8_t i = 4; i >= 0; i--) {
        uart0_putc((v & (1 << i)) ? '1' : '0');
    }
}

static void print_int16_as_decimal(int16_t value)
{
    char buffer[8];
    uint8_t i = 0;
    if (value < 0) { uart0_putc('-'); value = -value; }
    if (value == 0) { uart0_putc('0'); return; }
    while (value > 0 && i < sizeof(buffer)) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    while (i > 0) uart0_putc(buffer[--i]);
}

static void print_fixed_2(int16_t value_centi)
{
    if (value_centi < 0) { uart0_putc('-'); value_centi = -value_centi; }
    print_int16_as_decimal(value_centi / 100);
    uart0_putc('.');
    uint8_t frac = value_centi % 100;
    uart0_putc('0' + frac / 10);
    uart0_putc('0' + frac % 10);
}

static void tmag_init(void)
{
    twi1_write_register(TMAG_ADDR, DEVICE_CONFIG_1, 0x00);
    twi1_write_register(TMAG_ADDR, DEVICE_CONFIG_2, 0x02);
    twi1_write_register(TMAG_ADDR, SENSOR_CONFIG_1, 0x74);
    twi1_write_register(TMAG_ADDR, SENSOR_CONFIG_2, 0x03);
    twi1_write_register(TMAG_ADDR, T_CONFIG, 0x01);
}

static uint8_t tmag_read_data(int16_t *temp_centi, int16_t *bx_centi, int16_t *by_centi, int16_t *bz_centi, uint8_t *status)
{
    uint8_t data[9];
    if (!twi1_read_registers(TMAG_ADDR, T_MSB_RESULT, data, 9)) return 0;

    int16_t t_raw = ((int16_t)data[0] << 8) | data[1];
    int16_t x_raw = ((int16_t)data[2] << 8) | data[3];
    int16_t y_raw = ((int16_t)data[4] << 8) | data[5];
    int16_t z_raw = ((int16_t)data[6] << 8) | data[7];
    *status = data[8];

    *temp_centi = 2500 + (int16_t)(((int32_t)(t_raw - 17508) * 100) / 601);
    *bx_centi = (int16_t)(((int32_t)x_raw * 4000) / 32768);
    *by_centi = (int16_t)(((int32_t)y_raw * 4000) / 32768);
    *bz_centi = (int16_t)(((int32_t)z_raw * 4000) / 32768);
    return 1;
}

int main(void)
{
    uart0_init();
    twi1_init();
    board_id_init();
    _delay_ms(100);

    uart0_print("\r\nBU008 - Board ID + TMAG Data\r\n");
    uart0_print("DIP: bit0=PC0 bit1=PC1 bit2=PC2 bit3=PC3 bit4=PD2\r\n");
    uart0_print("UART0: 4800 baud\r\n");

    uint8_t device_id = 0;
    if (twi1_read_register(TMAG_ADDR, DEVICE_ID, &device_id)) {
        uart0_print("DEVICE_ID = 0x");
        uart0_print_hex8(device_id);
        uart0_print("\r\n");
    } else {
        uart0_print("DEVICE_ID read failed\r\n");
    }

    tmag_init();

    while (1) {
        uint8_t raw = board_id_read_raw();
        uint8_t board_id = board_id_read();
        int16_t temp, bx, by, bz;
        uint8_t status;

        uart0_print("RAW_DIP=0b");
        uart0_print_bin5(raw);
        uart0_print(" BOARD_ID=");
        print_int16_as_decimal(board_id);

        if (tmag_read_data(&temp, &bx, &by, &bz, &status)) {
            uart0_print(" TEMP="); print_fixed_2(temp); uart0_print(" C");
            uart0_print(" BX="); print_fixed_2(bx); uart0_print(" mT");
            uart0_print(" BY="); print_fixed_2(by); uart0_print(" mT");
            uart0_print(" BZ="); print_fixed_2(bz); uart0_print(" mT");
            uart0_print(" STATUS=0x"); uart0_print_hex8(status);
            uart0_print("\r\n");
        } else {
            uart0_print(" TMAG_READ_FAIL\r\n");
        }
        _delay_ms(1000);
    }
}

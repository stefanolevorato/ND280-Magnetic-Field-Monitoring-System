#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "uart0.h"
#include "twi1.h"

#define TMAG5273_ADDR              0x35
#define DEVICE_CONFIG_1            0x00
#define DEVICE_ID                  0x0D
#define MANUFACTURER_ID_LSB        0x0E
#define MANUFACTURER_ID_MSB        0x0F

static void print_fail_status(void)
{
    uart0_print("FAIL, TWI status = 0x");
    uart0_print_hex8(twi1_last_status());
    uart0_print("\r\n");
}

int main(void)
{
    uint8_t device_id = 0;
    uint8_t man_lsb = 0;
    uint8_t man_msb = 0;
    uint8_t config_before = 0;
    uint8_t config_after = 0;

    uart0_init();
    twi1_init();

    DDRD |= (1 << PD5) | (1 << PD6) | (1 << PD7);

    uart0_print("\r\n================================\r\n");
    uart0_print("BU006 - TMAG5273 Register Test\r\n");
    uart0_print("================================\r\n");
    uart0_print("UART0: 4800 baud\r\n");
    uart0_print("TWI1 : PE0=SDA1, PE1=SCL1\r\n");
    uart0_print("TMAG : address 0x35\r\n\r\n");

    while (1) {
        uart0_print("Reading ID registers...\r\n");

        if (!twi1_read_register(TMAG5273_ADDR, DEVICE_ID, &device_id)) {
            uart0_print("DEVICE_ID read ");
            print_fail_status();
        } else if (!twi1_read_register(TMAG5273_ADDR, MANUFACTURER_ID_LSB, &man_lsb)) {
            uart0_print("MANUFACTURER_ID_LSB read ");
            print_fail_status();
        } else if (!twi1_read_register(TMAG5273_ADDR, MANUFACTURER_ID_MSB, &man_msb)) {
            uart0_print("MANUFACTURER_ID_MSB read ");
            print_fail_status();
        } else {
            uart0_print("DEVICE_ID          = 0x");
            uart0_print_hex8(device_id);
            uart0_print("\r\n");

            uart0_print("MANUFACTURER_ID    = 0x");
            uart0_print_hex8(man_msb);
            uart0_print_hex8(man_lsb);
            uart0_print("\r\n");

            uart0_print("\r\nTesting register write/readback...\r\n");

            if (!twi1_read_register(TMAG5273_ADDR, DEVICE_CONFIG_1, &config_before)) {
                uart0_print("DEVICE_CONFIG_1 read-before ");
                print_fail_status();
            } else if (!twi1_write_register(TMAG5273_ADDR, DEVICE_CONFIG_1, 0x00)) {
                uart0_print("DEVICE_CONFIG_1 write ");
                print_fail_status();
            } else if (!twi1_read_register(TMAG5273_ADDR, DEVICE_CONFIG_1, &config_after)) {
                uart0_print("DEVICE_CONFIG_1 read-after ");
                print_fail_status();
            } else {
                uart0_print("DEVICE_CONFIG_1 before = 0x");
                uart0_print_hex8(config_before);
                uart0_print("\r\n");

                uart0_print("DEVICE_CONFIG_1 after  = 0x");
                uart0_print_hex8(config_after);
                uart0_print("\r\n");

                if (config_after == 0x00) {
                    uart0_print("PASS\r\n");
                    PORTD ^= (1 << PD7);
                } else {
                    uart0_print("FAIL: readback mismatch\r\n");
                }
            }
        }

        uart0_print("\r\n");
        _delay_ms(3000);
    }
}

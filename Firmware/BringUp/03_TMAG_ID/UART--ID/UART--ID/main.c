#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "uart0.h"
#include "twi1.h"

#define TMAG5273_ADDR       0x35
#define TMAG5273_DEVICE_ID  0x0D

int main(void)
{
    uint8_t id = 0;

    uart0_init();
    twi1_init();

    DDRD |= (1 << PD5) | (1 << PD6) | (1 << PD7);

    uart0_print("\r\n=================================\r\n");
    uart0_print("BU005 - TMAG5273 DEVICE_ID Read\r\n");
    uart0_print("=================================\r\n");
    uart0_print("UART0: 4800 baud\r\n");
    uart0_print("TWI1 : PE0=SDA1, PE1=SCL1\r\n");
    uart0_print("Device address: 0x35\r\n\r\n");

    while (1)
    {
        uart0_print("Reading DEVICE_ID register 0x0D...\r\n");

        if (twi1_read_register(TMAG5273_ADDR, TMAG5273_DEVICE_ID, &id))
        {
            uart0_print("DEVICE_ID = 0x");
            uart0_print_hex8(id);
            uart0_print("\r\nPASS\r\n\r\n");
            PORTD ^= (1 << PD7);
        }
        else
        {
            uart0_print("Read failed, TWI status = 0x");
            uart0_print_hex8(twi1_last_status());
            uart0_print("\r\nFAIL\r\n\r\n");
            PORTD ^= (1 << PD5);
        }

        _delay_ms(2000);
    }
}

/*
 * GccApplication1.c
 *
 * Created: 7/10/2026 10:38:20 AM
 * Author : levorato
 */ 

#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "uart0.h"

static void board_id_init(void)
{
    /*
     * DIP mapping:
     * bit 0: PC0, physical pin 23
     * bit 1: PC1, physical pin 24
     * bit 2: PC2, physical pin 25
     * bit 3: PC3, physical pin 26
     * bit 4: PD2, physical pin 32
     */

    DDRC &= ~((1 << PC0) |
              (1 << PC1) |
              (1 << PC2) |
              (1 << PC3));

    DDRD &= ~(1 << PD2);

    /*
     * Internal pull-ups disabled.
     * External 100 kohm pull-ups to 3.3 V are already installed.
     */
    PORTC &= ~((1 << PC0) |
               (1 << PC1) |
               (1 << PC2) |
               (1 << PC3));

    PORTD &= ~(1 << PD2);
}

static uint8_t board_id_read_raw(void)
{
    uint8_t raw = 0;

    if (PINC & (1 << PC0))
        raw |= (1 << 0);

    if (PINC & (1 << PC1))
        raw |= (1 << 1);

    if (PINC & (1 << PC2))
        raw |= (1 << 2);

    if (PINC & (1 << PC3))
        raw |= (1 << 3);

    if (PIND & (1 << PD2))
        raw |= (1 << 4);

    return raw;
}

static uint8_t board_id_read(void)
{
    /*
     * Open switch: input is HIGH.
     * ON/closed switch: input is connected to GND and reads LOW.
     */
    return (~board_id_read_raw()) & 0x1F;
}

static void uart0_print_binary5(uint8_t value)
{
    int8_t bit;

    for (bit = 4; bit >= 0; bit--)
    {
        if (value & (1 << bit))
            uart0_putc('1');
        else
            uart0_putc('0');
    }
}

static void uart0_print_uint8(uint8_t value)
{
    char buffer[3];
    uint8_t count = 0;

    if (value == 0)
    {
        uart0_putc('0');
        return;
    }

    while (value > 0)
    {
        buffer[count++] = '0' + (value % 10);
        value /= 10;
    }

    while (count > 0)
    {
        uart0_putc(buffer[--count]);
    }
}

int main(void)
{
    uart0_init();
    board_id_init();

    _delay_ms(100);

    uart0_print("\r\n");
    uart0_print("BU008A - DIP Board ID Test\r\n");
    uart0_print("PC0 PC1 PC2 PC3 PD2\r\n");
    uart0_print("\r\n");

    while (1)
    {
        uint8_t raw = board_id_read_raw();
        uint8_t board_id = board_id_read();

        uart0_print("RAW = 0b");
        uart0_print_binary5(raw);

        uart0_print("   ID = 0b");
        uart0_print_binary5(board_id);

        uart0_print("   DEC = ");
        uart0_print_uint8(board_id);

        uart0_print("\r\n");

        _delay_ms(500);
    }
}
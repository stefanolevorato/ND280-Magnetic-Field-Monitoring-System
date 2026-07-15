#include "config.h"
#include <avr/io.h>
#include <stdint.h>
#include "uart0.h"

#define UBRR_VALUE 12U  /* 4800 baud at F_CPU = 1 MHz, normal UART mode. */

void uart0_init(void)
{
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)(UBRR_VALUE & 0xFFU);
    UCSR0A = 0x00;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); /* 8N1 */
}

void uart0_putc(char c)
{
    while (!(UCSR0A & (1 << UDRE0))) {}
    UDR0 = c;
}

void uart0_print(const char *s)
{
    while (*s != '\0') uart0_putc(*s++);
}

void uart0_print_hex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    uart0_putc(hex[(value >> 4) & 0x0FU]);
    uart0_putc(hex[value & 0x0FU]);
}

void uart0_print_uint16(uint16_t value)
{
    char buffer[5];
    uint8_t count = 0;

    if (value == 0U) {
        uart0_putc('0');
        return;
    }

    while (value > 0U) {
        buffer[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (count > 0U) uart0_putc(buffer[--count]);
}

void uart0_print_uint32(uint32_t value)
{
    char buffer[10];
    uint8_t count = 0;

    if (value == 0UL) {
        uart0_putc('0');
        return;
    }

    while (value > 0UL) {
        buffer[count++] = (char)('0' + (value % 10UL));
        value /= 10UL;
    }

    while (count > 0U) uart0_putc(buffer[--count]);
}

void uart0_print_fixed_2(int32_t value_x100)
{
    if (value_x100 < 0) {
        uart0_putc('-');
        value_x100 = -value_x100;
    }

    uart0_print_uint32((uint32_t)(value_x100 / 100L));
    uart0_putc('.');

    uint8_t fraction = (uint8_t)(value_x100 % 100L);
    uart0_putc((char)('0' + (fraction / 10U)));
    uart0_putc((char)('0' + (fraction % 10U)));
}

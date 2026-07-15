#define F_CPU 1000000UL

#include <avr/io.h>
#include <stdint.h>
#include "uart0.h"

#define UBRR_VALUE 12  // 4800 baud at F_CPU=1 MHz, normal UART mode

void uart0_init(void)
{
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)(UBRR_VALUE & 0xFF);
    UCSR0A = 0x00;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8N1
}

void uart0_putc(char c)
{
    while (!(UCSR0A & (1 << UDRE0))) {}
    UDR0 = c;
}

void uart0_print(const char *s)
{
    while (*s) uart0_putc(*s++);
}

void uart0_print_hex8(uint8_t v)
{
    const char hex[] = "0123456789ABCDEF";
    uart0_putc(hex[(v >> 4) & 0x0F]);
    uart0_putc(hex[v & 0x0F]);
}

void uart0_print_hex16(uint16_t v)
{
    uart0_print_hex8((uint8_t)(v >> 8));
    uart0_print_hex8((uint8_t)(v & 0xFF));
}

void uart0_print_uint16(uint16_t v)
{
    char buf[6];
    uint8_t i = 0;
    if (v == 0) { uart0_putc('0'); return; }
    while (v > 0 && i < sizeof(buf)) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }
    while (i > 0) uart0_putc(buf[--i]);
}

void uart0_print_int16(int16_t v)
{
    if (v < 0) {
        uart0_putc('-');
        v = -v;
    }
    uart0_print_uint16((uint16_t)v);
}

void uart0_print_fixed_2(int32_t value_x100)
{
    if (value_x100 < 0) {
        uart0_putc('-');
        value_x100 = -value_x100;
    }

    uint16_t whole = (uint16_t)(value_x100 / 100);
    uint8_t frac = (uint8_t)(value_x100 % 100);

    uart0_print_uint16(whole);
    uart0_putc('.');
    uart0_putc('0' + (frac / 10));
    uart0_putc('0' + (frac % 10));
}

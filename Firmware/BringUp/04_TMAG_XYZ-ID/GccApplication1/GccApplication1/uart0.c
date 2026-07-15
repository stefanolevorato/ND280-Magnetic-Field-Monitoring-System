#define F_CPU 1000000UL

#include <avr/io.h>
#include <stdint.h>
#include "uart0.h"

#define BAUD 4800UL
#define UBRR_VALUE 12

void uart0_init(void)
{
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)(UBRR_VALUE);

    UCSR0A = 0x00;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart0_putc(char c)
{
    while (!(UCSR0A & (1 << UDRE0)));
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

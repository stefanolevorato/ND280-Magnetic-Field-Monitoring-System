#include "config.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>

#include "command_parser.h"
#include "uart0.h"

#define UBRR_VALUE 12U  /* 4800 baud at F_CPU = 1 MHz, normal UART mode. */

ISR(USART0_RX_vect)
{
    /* Read status before UDR0: reading UDR0 clears the UART error flags. */
    uint8_t status = UCSR0A;
    uint8_t value = UDR0;

    if ((status & ((1U << FE0) | (1U << DOR0) | (1U << UPE0))) != 0U) {
        command_parser_record_uart_error();
        return;
    }

    command_parser_receive_byte(value);
}

void uart0_init(void)
{
    /* RXD0/PD0 is idle-high. Keep it defined when the external TX is absent. */
    DDRD &= (uint8_t)~(1U << PD0);
    PORTD |= (1U << PD0);

    command_parser_init();

    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)(UBRR_VALUE & 0xFFU);
    UCSR0A = 0x00;
    UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);
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
    uint8_t count = 0U;

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
    uint8_t count = 0U;

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

void uart0_print_int32(int32_t value)
{
    if (value < 0L) {
        uart0_putc('-');
        value = -value;
    }
    uart0_print_uint32((uint32_t)value);
}

void uart0_print_fixed_2(int32_t value_x100)
{
    if (value_x100 < 0L) {
        uart0_putc('-');
        value_x100 = -value_x100;
    }

    uart0_print_uint32((uint32_t)(value_x100 / 100L));
    uart0_putc('.');

    {
        uint8_t fraction = (uint8_t)(value_x100 % 100L);
        uart0_putc((char)('0' + (fraction / 10U)));
        uart0_putc((char)('0' + (fraction % 10U)));
    }
}

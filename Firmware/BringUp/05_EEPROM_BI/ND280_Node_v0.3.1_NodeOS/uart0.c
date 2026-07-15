#include "config.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>
#include "uart0.h"

#define UBRR_VALUE 12U  /* 4800 baud at F_CPU = 1 MHz, normal UART mode. */
#define UART0_RX_BUFFER_MASK (UART0_RX_BUFFER_SIZE - 1U)

#if (UART0_RX_BUFFER_SIZE & UART0_RX_BUFFER_MASK) != 0
#error "UART0_RX_BUFFER_SIZE must be a power of two"
#endif

static volatile uint8_t rx_head = 0U;
static volatile uint8_t rx_tail = 0U;
static volatile uint8_t rx_overflow = 0U;
static volatile char rx_buffer[UART0_RX_BUFFER_SIZE];

ISR(USART_RX_vect)
{
    char value = (char)UDR0;
    uint8_t next = (uint8_t)((rx_head + 1U) & UART0_RX_BUFFER_MASK);

    if (next == rx_tail) {
        rx_overflow = 1U;
        return;
    }

    rx_buffer[rx_head] = value;
    rx_head = next;
}

void uart0_init(void)
{
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

    uint8_t fraction = (uint8_t)(value_x100 % 100L);
    uart0_putc((char)('0' + (fraction / 10U)));
    uart0_putc((char)('0' + (fraction % 10U)));
}

uint8_t uart0_readline(char *destination, uint8_t destination_size)
{
    static char line[UART0_COMMAND_MAX_LENGTH];
    static uint8_t length = 0U;

    while (rx_tail != rx_head) {
        char value = rx_buffer[rx_tail];
        rx_tail = (uint8_t)((rx_tail + 1U) & UART0_RX_BUFFER_MASK);

        if (value == '\r' || value == '\n') {
            if (length == 0U) continue;

            line[length] = '\0';
            uint8_t index = 0U;
            while (index + 1U < destination_size && line[index] != '\0') {
                destination[index] = line[index];
                index++;
            }
            destination[index] = '\0';
            length = 0U;
            return 1U;
        }

        if (value == '\b' || value == 0x7F) {
            if (length > 0U) length--;
            continue;
        }

        if (length + 1U < sizeof(line)) {
            line[length++] = value;
        } else {
            length = 0U;
            rx_overflow = 1U;
        }
    }

    return 0U;
}

uint8_t uart0_rx_overflowed(void)
{
    return rx_overflow;
}

void uart0_clear_rx_overflow(void)
{
    rx_overflow = 0U;
}

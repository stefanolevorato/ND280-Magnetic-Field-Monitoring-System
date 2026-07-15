/*
 * GccApplication1.c
 *
 * Created: 7/9/2026 2:01:30 PM
 * Author : levorato
 */ 

#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#define BAUD 4800UL
#define UBRR_VALUE 12
//#define BAUD 9600UL
//#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)

static void uart0_init(void)
{
	UBRR0H = (unsigned char)(UBRR_VALUE >> 8);
	UBRR0L = (unsigned char)(UBRR_VALUE);

	UCSR0A = 0x00;
	UCSR0B = (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8N1
}

static void uart0_putc(char c)
{
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = c;
}

static void uart0_print(const char *s)
{
	while (*s)
	{
		uart0_putc(*s++);
	}
}

int main(void)
{
	uart0_init();

	// PD5, PD6, PD7 LEDs as heartbeat
	DDRD |= (1 << PD5) | (1 << PD6) | (1 << PD7);

	while (1)
	{
		uart0_print("BU003 UART0 alive\r\n");

		PORTD ^= (1 << PD5);
		_delay_ms(500);
	}
}
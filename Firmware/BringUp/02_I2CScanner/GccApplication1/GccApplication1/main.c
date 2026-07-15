/*
 * GccApplication1.c
 *
 * Created: 7/9/2026 3:12:28 PM
 * Author : levorato
 */ 

#include <avr/io.h>


#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#define BAUD 4800UL
#define UBRR_VALUE 12

#define I2C_SCL_HZ 50000UL

static void uart0_init(void)
{
	UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
	UBRR0L = (uint8_t)(UBRR_VALUE);

	UCSR0A = 0x00;
	UCSR0B = (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

static void uart0_putc(char c)
{
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = c;
}

static void uart0_print(const char *s)
{
	while (*s) uart0_putc(*s++);
}

static void uart0_print_hex8(uint8_t v)
{
	const char hex[] = "0123456789ABCDEF";
	uart0_putc(hex[(v >> 4) & 0x0F]);
	uart0_putc(hex[v & 0x0F]);
}

static void twi1_init(void)
{
	// PE0 = SDA1, PE1 = SCL1
	// Enable pull-ups just in case external pull-ups are weak/missing
	PORTE |= (1 << PE0) | (1 << PE1);

	// TWI1 bit rate: approximately 50 kHz at F_CPU = 1 MHz
	TWSR1 = 0x00;   // prescaler = 1
	TWBR1 = 2;      // SCL approx 50 kHz

	TWCR1 = (1 << TWEN1);
}

static uint8_t twi1_start(void)
{
	TWCR1 = (1 << TWINT1) | (1 << TWSTA1) | (1 << TWEN1);
	while (!(TWCR1 & (1 << TWINT1)));

	uint8_t status = TWSR1 & 0xF8;
	return (status == 0x08 || status == 0x10);
}

static void twi1_stop(void)
{
	TWCR1 = (1 << TWINT1) | (1 << TWSTO1) | (1 << TWEN1);
}

static uint8_t twi1_probe(uint8_t address)
{
	if (!twi1_start())
	{
		twi1_stop();
		return 0;
	}

	TWDR1 = (address << 1);   // write address
	TWCR1 = (1 << TWINT1) | (1 << TWEN1);
	while (!(TWCR1 & (1 << TWINT1)));

	uint8_t status = TWSR1 & 0xF8;
	twi1_stop();

	return (status == 0x18);  // SLA+W transmitted, ACK received
}

int main(void)
{
	uart0_init();
	twi1_init();

	DDRD |= (1 << PD5) | (1 << PD6) | (1 << PD7);

	uart0_print("\r\nBU004 TWI1 I2C Scanner\r\n");
	uart0_print("UART0: 4800 baud\r\n");
	uart0_print("TWI1: PE0=SDA, PE1=SCL\r\n\r\n");

	while (1)
	{
		uint8_t found = 0;

		uart0_print("Scanning...\r\n");

		for (uint8_t addr = 1; addr < 127; addr++)
		{
			if (twi1_probe(addr))
			{
				uart0_print("Found device at 0x");
				uart0_print_hex8(addr);
				uart0_print("\r\n");
				found++;
			}
		}

		if (found == 0)
		{
			uart0_print("No I2C devices found\r\n");
		}

		uart0_print("Scan complete\r\n\r\n");

		PORTD ^= (1 << PD6);
		_delay_ms(3000);
	}
}


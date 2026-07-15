/*
 * GccApplication8.c
 *
 * Created: 7/9/2026 1:12:00 PM
 * Author : levorato
 */ 

#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
	// PD5, PD6, PD7 = ATmega328PB pins 9, 10, 11
	DDRD |= (1 << PD5) | (1 << PD6) | (1 << PD7);

	while (1)
	{
		PORTD |=  (1 << PD5);
		_delay_ms(500);
		PORTD &= ~(1 << PD5);

		PORTD |=  (1 << PD6);
		_delay_ms(500);
		PORTD &= ~(1 << PD6);

		PORTD |=  (1 << PD7);
		_delay_ms(500);
		PORTD &= ~(1 << PD7);

		_delay_ms(500);
	}
}


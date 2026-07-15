#include <avr/io.h>
#include <stdint.h>
#include "board_id.h"

void board_id_init(void)
{
    /*
     * DIP mapping:
     * bit 0 = PC0, physical pin 23
     * bit 1 = PC1, physical pin 24
     * bit 2 = PC2, physical pin 25
     * bit 3 = PC3, physical pin 26
     * bit 4 = PD2, physical pin 32
     */
    DDRC &= (uint8_t)~((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3));
    DDRD &= (uint8_t)~(1 << PD2);

    /* External 100 kohm pull-ups are fitted, so internal pull-ups stay off. */
    PORTC &= (uint8_t)~((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3));
    PORTD &= (uint8_t)~(1 << PD2);
}

uint8_t board_id_read_raw(void)
{
    uint8_t raw = 0;

    if (PINC & (1 << PC0)) raw |= (1 << 0);
    if (PINC & (1 << PC1)) raw |= (1 << 1);
    if (PINC & (1 << PC2)) raw |= (1 << 2);
    if (PINC & (1 << PC3)) raw |= (1 << 3);
    if (PIND & (1 << PD2)) raw |= (1 << 4);

    return raw;
}

uint8_t board_id_read(void)
{
    /* Open switch = HIGH; ON/closed switch = LOW. */
    return (uint8_t)((~board_id_read_raw()) & 0x1F);
}

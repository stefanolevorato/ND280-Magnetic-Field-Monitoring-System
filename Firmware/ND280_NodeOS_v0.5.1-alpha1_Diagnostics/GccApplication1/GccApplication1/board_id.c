#include <avr/io.h>
#include <stdint.h>
#include "board_id.h"

void board_id_init(void)
{
    /* bit0..bit3 = PC0..PC3; bit4 = PD2. External 100 kohm pull-ups. */
    DDRC &= ~((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3));
    DDRD &= ~(1 << PD2);

    PORTC &= ~((1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3));
    PORTD &= ~(1 << PD2);
}

uint8_t board_id_read(void)
{
    uint8_t raw = 0;

    if (PINC & (1 << PC0)) raw |= (1 << 0);
    if (PINC & (1 << PC1)) raw |= (1 << 1);
    if (PINC & (1 << PC2)) raw |= (1 << 2);
    if (PINC & (1 << PC3)) raw |= (1 << 3);
    if (PIND & (1 << PD2)) raw |= (1 << 4);

    /* Switch ON pulls low, so invert to obtain the board number. */
    return (uint8_t)((~raw) & 0x1FU);
}

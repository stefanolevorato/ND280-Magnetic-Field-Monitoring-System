#include <avr/io.h>
#include <stdint.h>
#include "twi1.h"

static uint8_t last_status = 0;

uint8_t twi1_last_status(void)
{
    return last_status;
}

static uint8_t twi1_get_status(void)
{
    last_status = TWSR1 & 0xF8;
    return last_status;
}

void twi1_init(void)
{
    // PE0 = SDA1, PE1 = SCL1. Enable weak pull-ups as backup.
    PORTE |= (1 << PE0) | (1 << PE1);

    // About 50 kHz SCL at F_CPU = 1 MHz, prescaler = 1.
    TWSR1 = 0x00;
    TWBR1 = 2;

    TWCR1 = (1 << TWEN1);
}

static uint8_t twi1_start(void)
{
    TWCR1 = (1 << TWINT1) | (1 << TWSTA1) | (1 << TWEN1);
    while (!(TWCR1 & (1 << TWINT1))) { }

    uint8_t status = twi1_get_status();
    return (status == 0x08 || status == 0x10);
}

static void twi1_stop(void)
{
    TWCR1 = (1 << TWINT1) | (1 << TWSTO1) | (1 << TWEN1);
}

static uint8_t twi1_write(uint8_t data)
{
    TWDR1 = data;
    TWCR1 = (1 << TWINT1) | (1 << TWEN1);
    while (!(TWCR1 & (1 << TWINT1))) { }
    return twi1_get_status();
}

static uint8_t twi1_read_nack(uint8_t *value)
{
    TWCR1 = (1 << TWINT1) | (1 << TWEN1);
    while (!(TWCR1 & (1 << TWINT1))) { }

    uint8_t status = twi1_get_status();
    *value = TWDR1;
    return status;
}

uint8_t twi1_probe(uint8_t address)
{
    if (!twi1_start()) {
        twi1_stop();
        return 0;
    }

    uint8_t status = twi1_write(address << 1);
    twi1_stop();

    return (status == 0x18);
}

uint8_t twi1_read_register(uint8_t address, uint8_t reg, uint8_t *value)
{
    if (!twi1_start()) {
        twi1_stop();
        return 0;
    }

    if (twi1_write(address << 1) != 0x18) {
        twi1_stop();
        return 0;
    }

    if (twi1_write(reg) != 0x28) {
        twi1_stop();
        return 0;
    }

    if (!twi1_start()) {
        twi1_stop();
        return 0;
    }

    if (twi1_write((address << 1) | 1) != 0x40) {
        twi1_stop();
        return 0;
    }

    if (twi1_read_nack(value) != 0x58) {
        twi1_stop();
        return 0;
    }

    twi1_stop();
    return 1;
}

#define F_CPU 1000000UL

#include <avr/io.h>
#include <stdint.h>
#include "twi1.h"

static uint8_t last_status = 0x00;

uint8_t twi1_last_status(void)
{
    return last_status;
}

void twi1_init(void)
{
    // PE0 = SDA1, PE1 = SCL1. Enable internal pullups as backup.
    PORTE |= (1 << PE0) | (1 << PE1);

    // About 50 kHz at F_CPU = 1 MHz: SCL = F_CPU / (16 + 2*TWBR*prescaler)
    TWSR1 = 0x00;       // prescaler = 1
    TWBR1 = 2;

    TWCR1 = (1 << TWEN1);
}

static uint8_t twi1_start_status(void)
{
    TWCR1 = (1 << TWINT1) | (1 << TWSTA1) | (1 << TWEN1);
    while (!(TWCR1 & (1 << TWINT1))) { }

    last_status = TWSR1 & 0xF8;
    return last_status;
}

static void twi1_stop(void)
{
    TWCR1 = (1 << TWINT1) | (1 << TWSTO1) | (1 << TWEN1);
}

static uint8_t twi1_write_byte(uint8_t data)
{
    TWDR1 = data;
    TWCR1 = (1 << TWINT1) | (1 << TWEN1);
    while (!(TWCR1 & (1 << TWINT1))) { }

    last_status = TWSR1 & 0xF8;
    return last_status;
}

static uint8_t twi1_read_nack(void)
{
    TWCR1 = (1 << TWINT1) | (1 << TWEN1);
    while (!(TWCR1 & (1 << TWINT1))) { }

    last_status = TWSR1 & 0xF8;
    return TWDR1;
}

uint8_t twi1_probe(uint8_t address)
{
    uint8_t s = twi1_start_status();
    if (s != 0x08 && s != 0x10) {
        twi1_stop();
        return TWI1_FAIL;
    }

    s = twi1_write_byte((uint8_t)(address << 1));
    twi1_stop();

    return (s == 0x18) ? TWI1_OK : TWI1_FAIL;
}

uint8_t twi1_read_register(uint8_t address, uint8_t reg, uint8_t *value)
{
    uint8_t s;

    s = twi1_start_status();
    if (s != 0x08 && s != 0x10) {
        twi1_stop();
        return TWI1_FAIL;
    }

    s = twi1_write_byte((uint8_t)(address << 1));       // SLA+W
    if (s != 0x18) {
        twi1_stop();
        return TWI1_FAIL;
    }

    s = twi1_write_byte(reg);                           // register pointer
    if (s != 0x28) {
        twi1_stop();
        return TWI1_FAIL;
    }

    s = twi1_start_status();                            // repeated START
    if (s != 0x10) {
        twi1_stop();
        return TWI1_FAIL;
    }

    s = twi1_write_byte((uint8_t)((address << 1) | 1)); // SLA+R
    if (s != 0x40) {
        twi1_stop();
        return TWI1_FAIL;
    }

    *value = twi1_read_nack();                          // one byte, NACK
    if (last_status != 0x58) {
        twi1_stop();
        return TWI1_FAIL;
    }

    twi1_stop();
    return TWI1_OK;
}

uint8_t twi1_write_register(uint8_t address, uint8_t reg, uint8_t value)
{
    uint8_t s;

    s = twi1_start_status();
    if (s != 0x08 && s != 0x10) {
        twi1_stop();
        return TWI1_FAIL;
    }

    s = twi1_write_byte((uint8_t)(address << 1));       // SLA+W
    if (s != 0x18) {
        twi1_stop();
        return TWI1_FAIL;
    }

    s = twi1_write_byte(reg);                           // register address
    if (s != 0x28) {
        twi1_stop();
        return TWI1_FAIL;
    }

    s = twi1_write_byte(value);                         // data
    if (s != 0x28) {
        twi1_stop();
        return TWI1_FAIL;
    }

    twi1_stop();
    return TWI1_OK;
}

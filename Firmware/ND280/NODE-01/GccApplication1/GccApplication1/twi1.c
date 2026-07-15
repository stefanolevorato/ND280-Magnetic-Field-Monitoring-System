#define F_CPU 1000000UL

#include <avr/io.h>
#include <stdint.h>
#include "twi1.h"

#define TWI_STATUS_MASK 0xF8

static uint8_t twi1_start(void)
{
    TWCR1 = (1 << TWINT1) | (1 << TWSTA1) | (1 << TWEN1);
    while (!(TWCR1 & (1 << TWINT1))) {}
    uint8_t st = TWSR1 & TWI_STATUS_MASK;
    return (st == 0x08 || st == 0x10);
}

static void twi1_stop(void)
{
    TWCR1 = (1 << TWINT1) | (1 << TWSTO1) | (1 << TWEN1);
}

static uint8_t twi1_write_byte(uint8_t data)
{
    TWDR1 = data;
    TWCR1 = (1 << TWINT1) | (1 << TWEN1);
    while (!(TWCR1 & (1 << TWINT1))) {}
    return TWSR1 & TWI_STATUS_MASK;
}

static uint8_t twi1_read_ack(void)
{
    TWCR1 = (1 << TWINT1) | (1 << TWEA1) | (1 << TWEN1);
    while (!(TWCR1 & (1 << TWINT1))) {}
    return TWDR1;
}

static uint8_t twi1_read_nack(void)
{
    TWCR1 = (1 << TWINT1) | (1 << TWEN1);
    while (!(TWCR1 & (1 << TWINT1))) {}
    return TWDR1;
}

void twi1_init(void)
{
    // PE0 = SDA1, PE1 = SCL1. Enable weak pullups as a safety net.
    PORTE |= (1 << PE0) | (1 << PE1);

    TWSR1 = 0x00; // prescaler = 1
    TWBR1 = 2;    // approx 50 kHz at F_CPU = 1 MHz
    TWCR1 = (1 << TWEN1);
}

uint8_t twi1_probe(uint8_t addr)
{
    if (!twi1_start()) { twi1_stop(); return 0; }
    uint8_t st = twi1_write_byte((uint8_t)(addr << 1));
    twi1_stop();
    return (st == 0x18);
}

uint8_t twi1_write_register(uint8_t addr, uint8_t reg, uint8_t value)
{
    if (!twi1_start()) { twi1_stop(); return 0; }
    if (twi1_write_byte((uint8_t)(addr << 1)) != 0x18) { twi1_stop(); return 0; }
    if (twi1_write_byte(reg) != 0x28) { twi1_stop(); return 0; }
    if (twi1_write_byte(value) != 0x28) { twi1_stop(); return 0; }
    twi1_stop();
    return 1;
}

uint8_t twi1_read_register(uint8_t addr, uint8_t reg, uint8_t *value)
{
    if (!twi1_start()) { twi1_stop(); return 0; }
    if (twi1_write_byte((uint8_t)(addr << 1)) != 0x18) { twi1_stop(); return 0; }
    if (twi1_write_byte(reg) != 0x28) { twi1_stop(); return 0; }
    if (!twi1_start()) { twi1_stop(); return 0; }
    if (twi1_write_byte((uint8_t)((addr << 1) | 1)) != 0x40) { twi1_stop(); return 0; }
    *value = twi1_read_nack();
    twi1_stop();
    return 1;
}

uint8_t twi1_read_block(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t len)
{
    if (len == 0) return 0;

    if (!twi1_start()) { twi1_stop(); return 0; }
    if (twi1_write_byte((uint8_t)(addr << 1)) != 0x18) { twi1_stop(); return 0; }
    if (twi1_write_byte(reg) != 0x28) { twi1_stop(); return 0; }
    if (!twi1_start()) { twi1_stop(); return 0; }
    if (twi1_write_byte((uint8_t)((addr << 1) | 1)) != 0x40) { twi1_stop(); return 0; }

    for (uint8_t i = 0; i < len; i++) {
        if (i == (len - 1)) buffer[i] = twi1_read_nack();
        else buffer[i] = twi1_read_ack();
    }

    twi1_stop();
    return 1;
}

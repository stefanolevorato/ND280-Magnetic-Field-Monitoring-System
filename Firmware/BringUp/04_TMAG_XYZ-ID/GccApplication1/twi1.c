#define F_CPU 1000000UL

#include <avr/io.h>
#include <stdint.h>
#include "twi1.h"

void twi1_init(void)
{
    TWSR1 = 0x00;
    TWBR1 = 2;
    TWCR1 = (1 << TWEN1);
}

uint8_t twi1_start(void)
{
    TWCR1 = (1 << TWINT1) | (1 << TWSTA1) | (1 << TWEN1);
    while (!(TWCR1 & (1 << TWINT1)));
    uint8_t status = TWSR1 & 0xF8;
    return (status == 0x08 || status == 0x10);
}

void twi1_stop(void)
{
    TWCR1 = (1 << TWINT1) | (1 << TWSTO1) | (1 << TWEN1);
}

uint8_t twi1_write(uint8_t data)
{
    TWDR1 = data;
    TWCR1 = (1 << TWINT1) | (1 << TWEN1);
    while (!(TWCR1 & (1 << TWINT1)));
    return (TWSR1 & 0xF8);
}

uint8_t twi1_read_ack(void)
{
    TWCR1 = (1 << TWINT1) | (1 << TWEN1) | (1 << TWEA1);
    while (!(TWCR1 & (1 << TWINT1)));
    return TWDR1;
}

uint8_t twi1_read_nack(void)
{
    TWCR1 = (1 << TWINT1) | (1 << TWEN1);
    while (!(TWCR1 & (1 << TWINT1)));
    return TWDR1;
}

uint8_t twi1_probe(uint8_t addr)
{
    if (!twi1_start()) { twi1_stop(); return 0; }
    uint8_t status = twi1_write(addr << 1);
    twi1_stop();
    return (status == 0x18);
}

uint8_t twi1_read_register(uint8_t addr, uint8_t reg, uint8_t *value)
{
    if (!twi1_start()) return 0;
    if (twi1_write(addr << 1) != 0x18) { twi1_stop(); return 0; }
    if (twi1_write(reg) != 0x28) { twi1_stop(); return 0; }
    if (!twi1_start()) return 0;
    if (twi1_write((addr << 1) | 1) != 0x40) { twi1_stop(); return 0; }
    *value = twi1_read_nack();
    twi1_stop();
    return 1;
}

uint8_t twi1_write_register(uint8_t addr, uint8_t reg, uint8_t value)
{
    if (!twi1_start()) return 0;
    if (twi1_write(addr << 1) != 0x18) { twi1_stop(); return 0; }
    if (twi1_write(reg) != 0x28) { twi1_stop(); return 0; }
    if (twi1_write(value) != 0x28) { twi1_stop(); return 0; }
    twi1_stop();
    return 1;
}

uint8_t twi1_read_registers(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len)
{
    if (len == 0) return 0;
    if (!twi1_start()) return 0;
    if (twi1_write(addr << 1) != 0x18) { twi1_stop(); return 0; }
    if (twi1_write(reg) != 0x28) { twi1_stop(); return 0; }
    if (!twi1_start()) return 0;
    if (twi1_write((addr << 1) | 1) != 0x40) { twi1_stop(); return 0; }
    for (uint8_t i = 0; i < len; i++) {
        data[i] = (i == (len - 1)) ? twi1_read_nack() : twi1_read_ack();
    }
    twi1_stop();
    return 1;
}

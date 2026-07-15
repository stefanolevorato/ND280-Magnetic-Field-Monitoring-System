#include "config.h"
#include <avr/io.h>
#include <stdint.h>
#include "twi1.h"

#define TWI_STATUS_MASK 0xF8U

static uint8_t twi1_start(void)
{
    TWCR1 = (1 << TWINT1) | (1 << TWSTA1) | (1 << TWEN1);
    while (!(TWCR1 & (1 << TWINT1))) {}

    uint8_t status = TWSR1 & TWI_STATUS_MASK;
    return (status == 0x08U || status == 0x10U);
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
    /* TWI1: PE0 = SDA1, PE1 = SCL1. */
    PORTE |= (1 << PE0) | (1 << PE1);
    TWSR1 = 0x00;
    TWBR1 = 2;  /* Approximately 50 kHz at 1 MHz CPU clock. */
    TWCR1 = (1 << TWEN1);
}

uint8_t twi1_probe(uint8_t address)
{
    if (!twi1_start()) { twi1_stop(); return 0; }
    uint8_t status = twi1_write_byte((uint8_t)(address << 1));
    twi1_stop();
    return status == 0x18U;
}

uint8_t twi1_write_register(uint8_t address, uint8_t reg, uint8_t value)
{
    if (!twi1_start()) { twi1_stop(); return 0; }
    if (twi1_write_byte((uint8_t)(address << 1)) != 0x18U) { twi1_stop(); return 0; }
    if (twi1_write_byte(reg) != 0x28U) { twi1_stop(); return 0; }
    if (twi1_write_byte(value) != 0x28U) { twi1_stop(); return 0; }
    twi1_stop();
    return 1;
}

uint8_t twi1_read_register(uint8_t address, uint8_t reg, uint8_t *value)
{
    if (!twi1_start()) { twi1_stop(); return 0; }
    if (twi1_write_byte((uint8_t)(address << 1)) != 0x18U) { twi1_stop(); return 0; }
    if (twi1_write_byte(reg) != 0x28U) { twi1_stop(); return 0; }
    if (!twi1_start()) { twi1_stop(); return 0; }
    if (twi1_write_byte((uint8_t)((address << 1) | 1U)) != 0x40U) { twi1_stop(); return 0; }
    *value = twi1_read_nack();
    twi1_stop();
    return 1;
}

uint8_t twi1_read_block(uint8_t address, uint8_t reg, uint8_t *buffer, uint8_t length)
{
    if (length == 0U) return 0;

    if (!twi1_start()) { twi1_stop(); return 0; }
    if (twi1_write_byte((uint8_t)(address << 1)) != 0x18U) { twi1_stop(); return 0; }
    if (twi1_write_byte(reg) != 0x28U) { twi1_stop(); return 0; }
    if (!twi1_start()) { twi1_stop(); return 0; }
    if (twi1_write_byte((uint8_t)((address << 1) | 1U)) != 0x40U) { twi1_stop(); return 0; }

    for (uint8_t index = 0; index < length; index++) {
        buffer[index] = (index == (uint8_t)(length - 1U))
                      ? twi1_read_nack()
                      : twi1_read_ack();
    }

    twi1_stop();
    return 1;
}

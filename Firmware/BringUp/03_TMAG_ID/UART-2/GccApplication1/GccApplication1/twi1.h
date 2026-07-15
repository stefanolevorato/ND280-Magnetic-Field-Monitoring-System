#ifndef TWI1_H
#define TWI1_H

#include <stdint.h>

#define TWI1_OK 1
#define TWI1_FAIL 0

void twi1_init(void);
uint8_t twi1_probe(uint8_t address);
uint8_t twi1_read_register(uint8_t address, uint8_t reg, uint8_t *value);
uint8_t twi1_write_register(uint8_t address, uint8_t reg, uint8_t value);
uint8_t twi1_last_status(void);

#endif

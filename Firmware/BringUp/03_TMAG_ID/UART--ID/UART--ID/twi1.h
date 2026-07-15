#ifndef TWI1_H
#define TWI1_H

#include <stdint.h>

void twi1_init(void);
uint8_t twi1_probe(uint8_t address);
uint8_t twi1_read_register(uint8_t address, uint8_t reg, uint8_t *value);
uint8_t twi1_last_status(void);

#endif

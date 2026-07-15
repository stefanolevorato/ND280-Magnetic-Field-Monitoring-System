#ifndef TWI1_H
#define TWI1_H

#include <stdint.h>

void twi1_init(void);
uint8_t twi1_start(void);
void twi1_stop(void);
uint8_t twi1_write(uint8_t data);
uint8_t twi1_read_ack(void);
uint8_t twi1_read_nack(void);
uint8_t twi1_probe(uint8_t addr);
uint8_t twi1_read_register(uint8_t addr, uint8_t reg, uint8_t *value);
uint8_t twi1_write_register(uint8_t addr, uint8_t reg, uint8_t value);
uint8_t twi1_read_registers(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);

#endif

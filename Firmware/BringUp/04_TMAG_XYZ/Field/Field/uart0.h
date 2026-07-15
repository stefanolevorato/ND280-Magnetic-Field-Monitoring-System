#ifndef UART0_H
#define UART0_H

#include <stdint.h>

void uart0_init(void);
void uart0_putc(char c);
void uart0_print(const char *s);
void uart0_print_hex8(uint8_t v);
void uart0_print_hex16(uint16_t v);
void uart0_print_int16(int16_t v);
void uart0_print_uint16(uint16_t v);
void uart0_print_fixed_2(int32_t value_x100);

#endif

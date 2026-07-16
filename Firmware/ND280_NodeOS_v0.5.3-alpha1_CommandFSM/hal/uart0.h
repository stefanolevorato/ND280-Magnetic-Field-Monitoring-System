#ifndef UART0_H
#define UART0_H

#include <stdint.h>

void uart0_init(void);
void uart0_putc(char c);
void uart0_print(const char *s);
void uart0_print_hex8(uint8_t value);
void uart0_print_uint16(uint16_t value);
void uart0_print_uint32(uint32_t value);
void uart0_print_int32(int32_t value);
void uart0_print_fixed_2(int32_t value_x100);

#endif

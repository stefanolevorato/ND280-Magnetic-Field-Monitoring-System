#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <stdint.h>

/* Parser error flags. They are diagnostic events, not fatal conditions. */
#define COMMAND_PARSER_ERROR_NONE       0x00U
#define COMMAND_PARSER_ERROR_TOO_LONG   0x01U
#define COMMAND_PARSER_ERROR_UART_FRAME 0x02U
#define COMMAND_PARSER_ERROR_BUSY       0x04U

void command_parser_init(void);

/* Called only from the USART0 RX interrupt. */
void command_parser_receive_byte(uint8_t value);
void command_parser_record_uart_error(void);

/* Called from the application context. Returns 1 for one complete command. */
uint8_t command_parser_get_command(char *destination, uint8_t destination_size);

/* Atomically returns and clears accumulated parser error flags. */
uint8_t command_parser_take_errors(void);

#endif

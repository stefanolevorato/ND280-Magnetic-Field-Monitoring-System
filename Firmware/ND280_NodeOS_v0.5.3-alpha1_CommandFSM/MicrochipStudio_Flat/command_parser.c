#include <avr/interrupt.h>
#include <stdint.h>

#include "command_parser.h"
#include "config.h"

typedef enum
{
    PARSER_WAIT_START = 0,
    PARSER_RECEIVE,
    PARSER_DISCARD
} parser_state_t;

static volatile parser_state_t parser_state;
static volatile char command_buffer[COMMAND_MAX_LENGTH];
static volatile uint8_t command_length;
static volatile uint8_t command_ready;
static volatile uint8_t parser_errors;

void command_parser_init(void)
{
    parser_state = PARSER_WAIT_START;
    command_length = 0U;
    command_ready = 0U;
    parser_errors = COMMAND_PARSER_ERROR_NONE;
}

void command_parser_record_uart_error(void)
{
    parser_errors |= COMMAND_PARSER_ERROR_UART_FRAME;
    parser_state = PARSER_WAIT_START;
    command_length = 0U;
}

void command_parser_receive_byte(uint8_t value)
{
    /* A complete command is waiting for the main loop. Do not overwrite it. */
    if (command_ready != 0U) {
        if (value == (uint8_t)COMMAND_START_CHARACTER) {
            parser_errors |= COMMAND_PARSER_ERROR_BUSY;
        }
        return;
    }

    switch (parser_state) {
    case PARSER_WAIT_START:
        if (value == (uint8_t)COMMAND_START_CHARACTER) {
            command_length = 0U;
            parser_state = PARSER_RECEIVE;
        }
        break;

    case PARSER_RECEIVE:
        if (value == (uint8_t)COMMAND_START_CHARACTER) {
            /* A new start marker re-synchronizes an incomplete command. */
            command_length = 0U;
            break;
        }

        if (value == '\r' || value == '\n') {
            if (command_length > 0U) {
                command_buffer[command_length] = '\0';
                command_ready = 1U;
            }
            command_length = 0U;
            parser_state = PARSER_WAIT_START;
            break;
        }

        /* Accept printable 7-bit ASCII only. */
        if (value >= 0x20U && value <= 0x7EU) {
            if (command_length + 1U < COMMAND_MAX_LENGTH) {
                command_buffer[command_length++] = (char)value;
            } else {
                parser_errors |= COMMAND_PARSER_ERROR_TOO_LONG;
                command_length = 0U;
                parser_state = PARSER_DISCARD;
            }
        } else {
            command_length = 0U;
            parser_state = PARSER_DISCARD;
        }
        break;

    case PARSER_DISCARD:
    default:
        if (value == (uint8_t)COMMAND_START_CHARACTER) {
            command_length = 0U;
            parser_state = PARSER_RECEIVE;
        } else if (value == '\r' || value == '\n') {
            command_length = 0U;
            parser_state = PARSER_WAIT_START;
        }
        break;
    }
}

uint8_t command_parser_get_command(char *destination, uint8_t destination_size)
{
    uint8_t saved_sreg;
    uint8_t index;

    if (destination == 0 || destination_size == 0U) return 0U;

    saved_sreg = SREG;
    cli();

    if (command_ready == 0U) {
        SREG = saved_sreg;
        return 0U;
    }

    index = 0U;
    while (index + 1U < destination_size && command_buffer[index] != '\0') {
        destination[index] = command_buffer[index];
        index++;
    }
    destination[index] = '\0';
    command_ready = 0U;

    SREG = saved_sreg;
    return 1U;
}

uint8_t command_parser_take_errors(void)
{
    uint8_t saved_sreg;
    uint8_t errors;

    saved_sreg = SREG;
    cli();
    errors = parser_errors;
    parser_errors = COMMAND_PARSER_ERROR_NONE;
    SREG = saved_sreg;

    return errors;
}

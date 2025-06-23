#ifndef MSG_PARSER_H
#define MSG_PARSER_H

#include "types.h"

types_error_code_e msg_parser_run(uint8_t * p_data, const uint16_t len);

#endif
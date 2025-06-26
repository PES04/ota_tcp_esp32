#ifndef MSG_PARSER_H
#define MSG_PARSER_H

#include <stdbool.h>

#include "types.h"


#define MSG_PARSER_BUF_LEN_BYTES    (10U)


types_error_code_e msg_parser_init(void);

types_error_code_e msg_parser_run(const uint8_t * p_data, const uint16_t len, uint32_t * p_out_bytes_read);

void msg_parser_clean(void);

types_error_code_e msg_parser_build_firmware_ack(uint8_t * p_buffer, const uint8_t len, uint8_t * p_out_len);

types_error_code_e msg_parser_build_ota_ack(uint8_t * p_buffer, 
                                            const uint8_t len, 
                                            const bool status,
                                            const uint32_t bytes_read, 
                                            uint8_t * p_out_len);

#endif
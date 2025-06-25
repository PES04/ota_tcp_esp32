#include <stdio.h>
#include <string.h>

#include "msg_parser.h"
#include "ota_manager.h"


#define FIRMWARE_LEN_SIZE_IN_BYTES          (4U)
#define HASH_SIZE_IN_BYTES                  (32U)
#define HEADER_SIZE_IN_BYTES                (FIRMWARE_LEN_SIZE_IN_BYTES + HASH_SIZE_IN_BYTES)


typedef enum {  
    READ_HEADER,
    START_OTA,
    WRITE_FIRMWARE
} msg_parser_states_e;


static void parse_header(uint8_t * p_data, uint32_t * p_firmware_size, uint8_t * p_hash);
static types_error_code_e ota_compare_hashes(uint8_t *sent_hash, uint8_t *calc_hash);

types_error_code_e msg_parser_run(uint8_t * p_data, const uint16_t len)
{
    static msg_parser_states_e state = READ_HEADER;

    static uint32_t firmware_size = 0U;
    static uint32_t firmware_bytes_read = 0U;
    static uint8_t hash[HASH_SIZE_IN_BYTES] = {};

    types_error_code_e status = ERR_CODE_IN_PROGRESS;

    switch (state)
    {
        case READ_HEADER:
            if (len == HEADER_SIZE_IN_BYTES)
            {
                parse_header(p_data, &firmware_size, hash);

                state = START_OTA;
            }
        break;

        case START_OTA:
            /* Start OTA */
            ota_process_init(firmware_size);
            state = WRITE_FIRMWARE;
            /* Fallthrough */

        case WRITE_FIRMWARE:
        {
            firmware_bytes_read += len;

            /* Call OTA Write */
            types_error_code_e err = ota_process_write_block(p_data, len);

            if (err != ERR_CODE_OK) {
                status = err;
            }

            if (firmware_bytes_read == firmware_size) { // Finished firmware transmission
                
                if (err == ERR_CODE_OK) {
                    uint8_t calc_hash[HASH_SIZE_IN_BYTES] = {};

                    /* Call OTA end */
                    err = ota_process_end(calc_hash);
                    if (err == ERR_CODE_OK) {
                        err = ota_compare_hashes(hash, calc_hash);     
                    }

                }
            
                /* Clean parameters */
                firmware_size = 0;
                firmware_bytes_read = 0;
                memset(hash, 0, sizeof(hash));

                state = READ_HEADER;
                status = err;
            }
        }
        break;

        default:
            status = ERR_CODE_FAIL;
        break;
    }

    return status;
}

static void parse_header(uint8_t * p_data, uint32_t * p_firmware_size, uint8_t * p_hash)
{
    for (uint8_t i = 0; i < FIRMWARE_LEN_SIZE_IN_BYTES; i++)
    {
        *p_firmware_size |= ((uint32_t)p_data[i]) << (8U * i);
    }

    memcpy(p_hash, p_data + FIRMWARE_LEN_SIZE_IN_BYTES, HASH_SIZE_IN_BYTES);
}

static types_error_code_e ota_compare_hashes(uint8_t *sent_hash, uint8_t *calc_hash) {
    for (int i = 0; i < HASH_SIZE_IN_BYTES; i++) {
        if (sent_hash[i] != calc_hash[i]) {
            return ERR_CODE_FAIL;
        }
    }

    return ERR_CODE_OK;
}
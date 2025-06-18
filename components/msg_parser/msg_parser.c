#include <stdio.h>
#include <string.h>

#include "types.h"
#include "msg_parser.h"


#define FIRMWARE_LEN_SIZE_IN_BYTES          (4U)
#define HASH_SIZE_IN_BYTES                  (32U)
#define HEADER_SIZE_IN_BYTES                (FIRMWARE_LEN_SIZE_IN_BYTES + HASH_SIZE_IN_BYTES)


typedef enum {  
    READ_HEADER,
    START_OTA,
    WRITE_FIRMWARE,
    END_OTA,
    CLEAN_PARAMS
} msg_parser_states_e;


static void parse_header(uint8_t * p_data, uint32_t * p_firmware_size, uint8_t * p_hash);


types_ret_state_machine_e msg_parser_run(uint8_t * p_data, const uint16_t len)
{
    static msg_parser_states_e state = READ_HEADER;

    static uint32_t firmware_size = 0;
    static uint32_t firmware_bytes_read = 0;
    static uint8_t hash[HASH_SIZE_IN_BYTES] = {};

    types_ret_state_machine_e status = SM_IN_PROGRESS;

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
            /* Start OTA here */
            state = WRITE_FIRMWARE;
            /* Fallthrough */

        case WRITE_FIRMWARE:
            firmware_bytes_read += len;
            
            if (firmware_bytes_read > firmware_size)
            {
                status = SM_ERROR;
                break;
            }

            /* Apply OTA here */
            if (1 /* OTA not end */)
            {
                break;
            }
            /* Fallthrough */

        case END_OTA:
            /* Stop OTA here */
        //Fallthrough

        case CLEAN_PARAMS:
            firmware_size = 0;
            firmware_bytes_read = 0;
            memset(hash, 0, sizeof(hash));
        break;

        default:
            state = SM_ERROR;
        break;
    }

    return state;
}

static void parse_header(uint8_t * p_data, uint32_t * p_firmware_size, uint8_t * p_hash)
{
    for (uint8_t i = 0; i < FIRMWARE_LEN_SIZE_IN_BYTES; i++)
    {
        *p_firmware_size |= ((uint32_t)p_data[i]) << (8U * i);
    }

    memcpy(p_hash, p_data + FIRMWARE_LEN_SIZE_IN_BYTES, HASH_SIZE_IN_BYTES);
}

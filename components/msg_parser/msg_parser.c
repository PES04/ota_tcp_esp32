#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "msg_parser.h"
#include "ota_manager.h"


#define FIRMWARE_LEN_SIZE_IN_BYTES          (4U)
#define HASH_SIZE_IN_BYTES                  (32U)
#define HEADER_SIZE_IN_BYTES                (FIRMWARE_LEN_SIZE_IN_BYTES + HASH_SIZE_IN_BYTES)

/* ------------- OTA ACK PARAMETERS ------------- */
#define OTA_ACK_SIZE_IN_BYTES               (6U)
#define OTA_ACK_LEN_SIZE_IN_BYTES           (4U)
#define OTA_ACK_ERR_SIZE_IN_BYTE            (2U)


typedef enum {  
    READ_HEADER,
    START_OTA,
    WRITE_FIRMWARE
} msg_parser_states_e;

typedef struct {
    msg_parser_states_e state;
    uint32_t firmware_size;
    uint32_t firmware_bytes_read;
    uint8_t hash[HASH_SIZE_IN_BYTES];
    SemaphoreHandle_t semaphore;
} state_machine_params_t;

static state_machine_params_t state_machine_instance = {};


static void parse_header(const uint8_t * p_data, uint32_t * p_firmware_size, uint8_t * p_hash);


types_error_code_e msg_parser_init(void)
{
    vSemaphoreCreateBinary(state_machine_instance.semaphore);
    if (state_machine_instance.semaphore == NULL)
    {
        return ERR_CODE_FAIL;
    }
    
    state_machine_instance.state = READ_HEADER;

    return ERR_CODE_OK;
}

types_error_code_e msg_parser_run(const uint8_t * p_data, const uint16_t len, uint32_t * p_out_bytes_read)
{
    xSemaphoreTake(state_machine_instance.semaphore, portMAX_DELAY);

    types_error_code_e status = ERR_CODE_IN_PROGRESS;
    
    *p_out_bytes_read = UINT32_MAX;

    switch (state_machine_instance.state)
    {
        case READ_HEADER:
            if (len == HEADER_SIZE_IN_BYTES)
            {
                parse_header(p_data, &state_machine_instance.firmware_size, state_machine_instance.hash);

                state_machine_instance.state = START_OTA;
            }
        break;

        case START_OTA:
            /* Start OTA */
            ota_process_init(state_machine_instance.firmware_size);
            state_machine_instance.state = WRITE_FIRMWARE;
            /* Fallthrough */

        case WRITE_FIRMWARE:
        {
            state_machine_instance.firmware_bytes_read += len;

            /* Call OTA Write */
            types_error_code_e err = ota_process_write_block(p_data, len);

            if ((err == ERR_CODE_OK) || (err == ERR_CODE_FAIL))
            {
                /* Externalize firmware bytes read */
                *p_out_bytes_read = state_machine_instance.firmware_bytes_read;

                /* Clean parameters */
                state_machine_instance.firmware_size = 0;
                state_machine_instance.firmware_bytes_read = 0;
                memset(state_machine_instance.hash, 0, sizeof(state_machine_instance.hash));

                /* Call OTA end here */

                state_machine_instance.state = READ_HEADER;
                status = err;
            }
        }
        break;

        default:
            status = ERR_CODE_FAIL;
        break;
    }

    xSemaphoreGive(state_machine_instance.semaphore);

    return status;
}

void msg_parser_clean(void)
{
    xSemaphoreTake(state_machine_instance.semaphore, portMAX_DELAY);

    state_machine_instance.state = READ_HEADER;
    state_machine_instance.firmware_size = 0;
    state_machine_instance.firmware_bytes_read = 0;
    memset(state_machine_instance.hash, 0, sizeof(state_machine_instance.hash));

    xSemaphoreGive(state_machine_instance.semaphore);
}

types_error_code_e msg_parser_build_firmware_ack(uint8_t * p_buffer, const uint8_t len, uint8_t * p_out_len)
{
    if (len != MSG_PARSER_BUF_LEN_BYTES)
    {
        return ERR_CODE_INVALID_PARAM;
    }
    
    uint8_t msg[] = {0xA3, 0x5F, 0x1C, 0xE7};
    
    memcpy(p_buffer, msg, sizeof(msg));
    *p_out_len = sizeof(msg);

    return ERR_CODE_OK;
}

types_error_code_e msg_parser_build_ota_ack(uint8_t * p_buffer, 
                                            const uint8_t len, 
                                            const bool status,
                                            const uint32_t bytes_read, 
                                            uint8_t * p_out_len)
{
    if (len != MSG_PARSER_BUF_LEN_BYTES)
    {
        return ERR_CODE_INVALID_PARAM;
    }
    
    uint8_t msg[OTA_ACK_SIZE_IN_BYTES] = {};
    for (uint8_t i = 0; i < OTA_ACK_LEN_SIZE_IN_BYTES; i++)
    {
        msg[i] = (bytes_read >> (i * 8U)) & 0xFF;
    }

    uint16_t err_code = (status == true) ? 100U : 200U;
    for (uint8_t i = 0; i < OTA_ACK_ERR_SIZE_IN_BYTE; i++)
    {
        msg[i + OTA_ACK_LEN_SIZE_IN_BYTES] = (err_code >> (i * 8U)) & 0xFF;
    }

    memcpy(p_buffer, msg, sizeof(msg));
    *p_out_len = sizeof(msg);

    return ERR_CODE_OK;
}

static void parse_header(const uint8_t * p_data, uint32_t * p_firmware_size, uint8_t * p_hash)
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
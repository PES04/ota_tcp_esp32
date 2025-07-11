#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "ota_manager.h"
#include "sys_feedback.h"
#include "msg_parser.h"

#define FIRMWARE_LEN_SIZE_IN_BYTES          (4U)
#define HASH_SIZE_IN_BYTES                  (32U)
#define HEADER_SIZE_IN_BYTES                (FIRMWARE_LEN_SIZE_IN_BYTES + HASH_SIZE_IN_BYTES)

/* ------------- OTA ACK PARAMETERS ------------- */
#define OTA_ACK_SIZE_IN_BYTES               (6U)
#define OTA_ACK_LEN_SIZE_IN_BYTES           (4U)
#define OTA_ACK_ERR_SIZE_IN_BYTE            (2U)
#define OTA_ACK_OK_CODE                     (100U)
#define OTA_ACK_FAIL_CODE                   (100U)

/* ---------- FIRMWARE ACK PARAMETERS ---------- */
#define FIRMWARE_ACK_SIZE_IN_BYTES          (4U)


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

/**
 * @brief Initialize the msg_parser component
 * 
 * @return types_error_code_e 
 */
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

/**
 * @brief Msg_parser state machine, responsible for parse the incoming messagens
 * 
 * @param p_data [in]: Message data buffer
 * @param len [in]: Message data buffer length
 * @param p_out_bytes_read [out]: Number os bytes read
 * @return types_error_code_e 
 */
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
        {
            if (ota_process_init(state_machine_instance.firmware_size, state_machine_instance.hash) != ERR_CODE_OK)
            {
                ota_process_end(false);
                
                types_error_code_e err = ota_process_init(state_machine_instance.firmware_size, state_machine_instance.hash);
                if (err != ERR_CODE_OK)
                {
                    status = err;
                    break;
                }
            }

            sys_feedback_set_update_mode();
            
            state_machine_instance.state = WRITE_FIRMWARE;
        }
            /* Fallthrough */

        case WRITE_FIRMWARE:
        {
            state_machine_instance.firmware_bytes_read += len;

            types_error_code_e err = ota_process_write_block(p_data, len);

            if ((err == ERR_CODE_OK) || (err == ERR_CODE_FAIL))
            {
                /* Externalize firmware bytes read */
                *p_out_bytes_read = state_machine_instance.firmware_bytes_read;

                /* Clean parameters */
                state_machine_instance.firmware_size = 0;
                state_machine_instance.firmware_bytes_read = 0;
                memset(state_machine_instance.hash, 0, sizeof(state_machine_instance.hash));

                err = (err == ERR_CODE_OK)? ota_process_end(true) : ota_process_end(false);
                
                sys_feedback_set_normal_mode();
                
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

/**
 * @brief Reset msg_parser state machine parameters
 * 
 */
void msg_parser_clean(void)
{
    xSemaphoreTake(state_machine_instance.semaphore, portMAX_DELAY);

    state_machine_instance.state = READ_HEADER;
    state_machine_instance.firmware_size = 0;
    state_machine_instance.firmware_bytes_read = 0;
    memset(state_machine_instance.hash, 0, sizeof(state_machine_instance.hash));

    sys_feedback_set_normal_mode();

    xSemaphoreGive(state_machine_instance.semaphore);
}

/**
 * @brief Build the firmware ack message
 * 
 * @param p_buffer [in]: Message data buffer
 * @param len [in]: Message data buffer length
 * @param p_out_len [out]: Built frame length
 * @return types_error_code_e 
 */
types_error_code_e msg_parser_build_firmware_ack(uint8_t * p_buffer, const uint8_t len, uint8_t * p_out_len)
{
    if (len < FIRMWARE_ACK_SIZE_IN_BYTES)
    {
        return ERR_CODE_INVALID_PARAM;
    }
    
    uint8_t msg[] = {0xA3, 0x5F, 0x1C, 0xE7};
    
    memcpy(p_buffer, msg, sizeof(msg));
    *p_out_len = sizeof(msg);

    return ERR_CODE_OK;
}

/**
 * @brief 
 * 
 * @param p_buffer [in]: Message data buffer
 * @param len [in]: Message data buffer length
 * @param status [in]: True for success e false for fail
 * @param bytes_read [in]: Number of bytes read
 * @param p_out_len [out]: Built frame length
 * @return types_error_code_e 
 */
types_error_code_e msg_parser_build_ota_ack(uint8_t * p_buffer, 
                                            const uint8_t len, 
                                            const bool status,
                                            const uint32_t bytes_read, 
                                            uint8_t * p_out_len)
{
    if (len < OTA_ACK_SIZE_IN_BYTES)
    {
        return ERR_CODE_INVALID_PARAM;
    }
    
    uint8_t msg[OTA_ACK_SIZE_IN_BYTES] = {};
    for (uint8_t i = 0; i < OTA_ACK_LEN_SIZE_IN_BYTES; i++)
    {
        msg[i] = (bytes_read >> (i * 8U)) & 0xFF;
    }

    uint16_t err_code = (status == true) ? OTA_ACK_OK_CODE : OTA_ACK_FAIL_CODE;
    for (uint8_t i = 0; i < OTA_ACK_ERR_SIZE_IN_BYTE; i++)
    {
        msg[i + OTA_ACK_LEN_SIZE_IN_BYTES] = (err_code >> (i * 8U)) & 0xFF;
    }

    memcpy(p_buffer, msg, sizeof(msg));
    *p_out_len = sizeof(msg);

    return ERR_CODE_OK;
}

/**
 * @brief Parse header message
 * 
 * @param p_data [in]: Message data buffer
 * @param p_firmware_size [out]: Firmware size received
 * @param p_hash [out]: Hash received
 */
static void parse_header(const uint8_t * p_data, uint32_t * p_firmware_size, uint8_t * p_hash)
{
    for (uint8_t i = 0; i < FIRMWARE_LEN_SIZE_IN_BYTES; i++)
    {
        *p_firmware_size |= ((uint32_t)p_data[i]) << (8U * i);
    }

    memcpy(p_hash, p_data + FIRMWARE_LEN_SIZE_IN_BYTES, HASH_SIZE_IN_BYTES);
}

#ifndef _OTA_MANAGER_H
#define _OTA_MANAGER_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "types.h"

types_error_code_e ota_process_init(size_t img_size);
types_error_code_e ota_process_write_block(const uint8_t *data, size_t data_len);
types_error_code_e ota_process_end(uint8_t *out_sha256);

void ota_check_rollback();

#endif
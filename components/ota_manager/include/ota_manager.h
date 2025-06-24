#ifndef _OTA_MANAGER_H
#define _OTA_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

esp_err_t ota_process_init(size_t img_size);
esp_err_t ota_process_write_block(const uint8_t *data, size_t data_len);
esp_err_t ota_process_end(uint8_t *out_sha256);

void ota_check_rollback();

#endif
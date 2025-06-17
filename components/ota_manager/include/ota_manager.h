#ifndef _OTA_MANAGER_H
#define _OTA_MANAGER_H

#include <stdint.h>
#include "esp_err.h"

void ota_rollback(void);

esp_err_t ota_write_firmware(const uint8_t *data, size_t len);

#endif
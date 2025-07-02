#ifndef _OTA_MANAGER_H
#define _OTA_MANAGER_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "types.h"

types_error_code_e ota_process_init(const size_t, const uint8_t*);
types_error_code_e ota_process_write_block(const uint8_t*, const size_t);
types_error_code_e ota_process_end(bool);

void ota_check_rollback(bool);

#endif
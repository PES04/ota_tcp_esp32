#ifndef AUTH_HMAC_H
#define AUTH_HMAC_H

#include <stdint.h>
#include <stdbool.h>

#include "types.h"


#define AUTH_HMAC_MAX_BUFFER_LEN        (48U)
#define AUTH_HMAC_NONCE_LEN             (16U)


types_error_code_e auth_hmac_set_hmac_psk(const uint8_t *key, const size_t len);

void auth_hmac_generate_nonce(uint8_t *nonce, size_t len);

bool auth_hmac_verify_response(const uint8_t *nonce, size_t nonce_len, const uint8_t *received_hmac, size_t received_len);

#endif
#ifndef AUTH_HMAC_H
#define AUTH_HMAC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define HMAC_SHARED_KEY   "private_shared_key_32_bytes_1234" //TODO: Save shared key in a secure way
#define HMAC_SHA256_LEN   (32U) // SHA-256 produces a 32-byte hash
#define NOUNCE_LEN        (16U)

void auth_hmac_generate_nounce(uint8_t *nounce, size_t len);
bool auth_hmac_verify_response(const uint8_t *nounce, size_t nounce_len, const uint8_t *received_hmac, size_t received_len);

#endif
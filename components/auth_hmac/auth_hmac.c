#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "esp_log.h"
#include "esp_random.h"
#include "mbedtls/md.h"
#include "auth_hmac.h"

static const char *tag = "AUTH_HMAC";

static struct {
    uint8_t val[AUTH_HMAC_MAX_BUFFER_LEN];
    size_t len;
    bool is_set;
} psk = { .is_set = false };

/**
 * @brief pre-shared key setter
 * 
 * @param key [in]: pre-shared key
 * @param len [in]: pre-shared key length in bytes
 */
types_error_code_e auth_hmac_set_hmac_psk(const uint8_t *key, const size_t len)
{  
  if (psk.is_set == true)
  {
    ESP_LOGE(tag, "----- Shared key already set -----");
    return ERR_CODE_NOT_ALLOWED;
  }
  
  if (len == 0 || len > AUTH_HMAC_MAX_BUFFER_LEN)
  {
    ESP_LOGE(tag, "----- Shared key invalid range -----");
    return ERR_CODE_INVALID_PARAM;
  }

  memcpy(psk.val, key, len);
  psk.len = len;
  psk.is_set = true;
  ESP_LOGI(tag, "----- Shared key has set -----");

  return ERR_CODE_OK;
}


/**
 * @brief Generate a random nonce for HMAC authentication
 * 
 * @param nonce [out]: Pointer to the buffer where the nonce will be stored
 * @param len [in]: Length of the nonce in bytes
 */
void auth_hmac_generate_nonce(uint8_t *nonce, size_t len)
{
  esp_fill_random(nonce, len); // Fill nonce param with random bytes
  ESP_LOGI(tag, "----- Nonce generated -----");
}

/**
 * @brief Verify the HMAC response using the nonce and shared key
 * 
 * @param nonce [in]: Pointer to the nonce used for HMAC generation
 * @param nonce_len [in]: Length of the nonce in bytes
 * @param received_hmac [in]: Pointer to the received HMAC response
 * @param received_len [in]: Length of the received HMAC response in bytes
 * @return true if the HMAC is valid, false otherwise
 */
bool auth_hmac_verify_response(const uint8_t *nonce, size_t nonce_len, const uint8_t *received_hmac, size_t received_len)
{
  ESP_LOGI(tag, "----- Starting HMAC verification -----");

  if (received_len != HMAC_SHA256_LEN) // Sanity check for HMAC length
  {
    ESP_LOGW(tag, "----- Invalid HMAC response length -----");
    return false;
  }

  uint8_t calculated_hmac[HMAC_SHA256_LEN] = {0};
  const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256); // Use SHA-256 for HMAC
  if (md_info == NULL)
  {
    ESP_LOGE(tag, "----- mbedtls_md_info_from_type failed -----");
    return false;
  }

  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);

  if (mbedtls_md_setup(&ctx, md_info, 1) != 0) // enable HMAC
  {
    ESP_LOGE(tag, "----- mbedtls_md_setup failed -----");
    mbedtls_md_free(&ctx);
    return false;
  }

  if (mbedtls_md_hmac_starts(&ctx, psk.val, psk.len) != 0 ||
      mbedtls_md_hmac_update(&ctx, nonce, nonce_len) != 0 || // Update with nonce
      mbedtls_md_hmac_finish(&ctx, calculated_hmac) != 0) // Compute HMAC
  {
    ESP_LOGE(tag, "----- Error during HMAC computation -----");
    mbedtls_md_free(&ctx);
    return false;
  }

  mbedtls_md_free(&ctx); 

  bool valid = memcmp(received_hmac, calculated_hmac, HMAC_SHA256_LEN) == 0; // Compare received HMAC with calculated HMAC memories

  if (valid)
    ESP_LOGI(tag, "----- HMAC successfully verified -----");
  else
    ESP_LOGW(tag, "----- Invalid HMAC -----");

  return valid;
}
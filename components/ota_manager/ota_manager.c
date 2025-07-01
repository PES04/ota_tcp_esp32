#include <string.h>
#include "ota_manager.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_log.h"
#include "mbedtls/sha256.h"

#define HASH_SIZE_IN_BYTES                  (32U)

static const char *TAG = "OTA";

static const esp_partition_t *ota_partition = NULL;
static esp_ota_handle_t ota_handle = 0;
static mbedtls_sha256_context sha_ctx;
static bool ota_in_progress = false;
static size_t fmw_size = 0;
static size_t updated_fmw_size = 0;
static uint8_t sent_hash[HASH_SIZE_IN_BYTES] = {0};

static int ota_process_compute_hash(uint8_t *out_sha256);
static types_error_code_e ota_compare_hashes(const uint8_t *sent_hash, const uint8_t *calc_hash);


types_error_code_e ota_process_init(const size_t img_size, const uint8_t* hash) {

    if (ota_in_progress) { // Update already in progress
        return ERR_CODE_NOT_ALLOWED;
    }

    fmw_size = img_size;
    memcpy(sent_hash, hash, HASH_SIZE_IN_BYTES);


    ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) { // Invalid OTA partition
        return ERR_CODE_FAIL; 
    }

    ESP_LOGI(TAG, "Iniciando OTA para partição: %s", ota_partition->label);

    // Allocates memory for the OTA partition
    ESP_ERROR_CHECK(esp_ota_begin(ota_partition, fmw_size, &ota_handle));

    // Initialize the context and starts the message digest computation
    mbedtls_sha256_init(&sha_ctx);
    mbedtls_sha256_starts(&sha_ctx, 0); // 0 for SHA-256

    ota_in_progress = true;
    return ERR_CODE_OK;
}

types_error_code_e ota_process_write_block(const uint8_t *data, const size_t data_len) {

    if (!ota_in_progress) { 
        return ERR_CODE_NOT_ALLOWED;
    }

    esp_err_t err = esp_ota_write(ota_handle, data, data_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro na escrita OTA: %s", esp_err_to_name(err));
        return ERR_CODE_FAIL;
    }

    // Updates SHA256 computation with buffer data
    mbedtls_sha256_update(&sha_ctx, data, data_len);

    updated_fmw_size += data_len;

    ESP_LOGI(TAG, "Updated firmware size: %zu / %zu", updated_fmw_size, fmw_size);

    if (updated_fmw_size < fmw_size) {
        return ERR_CODE_IN_PROGRESS;

    } else if (updated_fmw_size > fmw_size) {
        return ERR_CODE_FAIL;

    } else {
        uint8_t calc_hash[HASH_SIZE_IN_BYTES] = {};

        if (ota_process_compute_hash(calc_hash) != 0) {
            ESP_LOGE(TAG, "Falha ao computar hash SHA-256");
            return ERR_CODE_FAIL;
        }

        ESP_LOGI(TAG, "Hash recebido:");
        ESP_LOG_BUFFER_HEX(TAG, sent_hash, HASH_SIZE_IN_BYTES);
        ESP_LOGI(TAG, "Hash calculado:");
        ESP_LOG_BUFFER_HEX(TAG, calc_hash, HASH_SIZE_IN_BYTES);

        
        return (ota_compare_hashes(sent_hash, calc_hash));     
    }
}

int ota_process_compute_hash(uint8_t *out_sha256) {
    
    // Finishes SHA256 computation
    return mbedtls_sha256_finish(&sha_ctx, out_sha256);

}

types_error_code_e ota_process_end() {

    if (!ota_in_progress) { 
        return ERR_CODE_NOT_ALLOWED;
    }

    // Finish OTA update
    ESP_ERROR_CHECK(esp_ota_end(ota_handle));
    
    // Free memory allocated for the context
    mbedtls_sha256_free(&sha_ctx);
    
    ota_in_progress = false;
    updated_fmw_size = 0;

    if (esp_ota_set_boot_partition(ota_partition) != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao configurar nova partição OTA");
        return ERR_CODE_FAIL;
    }

    return ERR_CODE_OK;
}

static types_error_code_e ota_compare_hashes(const uint8_t *sent_hash, const uint8_t *calc_hash) {
    ESP_LOGI(TAG, "OTA process compare hashes");

    for (int i = 0; i < HASH_SIZE_IN_BYTES; i++) {
        if (sent_hash[i] != calc_hash[i]) {
            ESP_LOGE(TAG, "Hashes diferentes");
            return ERR_CODE_FAIL;
        }
    }

    return ERR_CODE_OK;
}

void ota_check_rollback() {
#if defined(CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE)

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;

    if (esp_ota_get_state_partition(running, &state) == ESP_OK) {

        if (state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "Verificação do firmware pendente.");

            if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) { // Cancels rollback
                ESP_LOGI(TAG, "Firmware marcado como válido. Rollback cancelado com sucesso.");
                
            } else { // Rolls back to the previously workable app and restarts ESP
                ESP_LOGE(TAG, "Falha ao marcar firmware como válido.");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }

        } else {
            ESP_LOGI(TAG, "Firmware já validado.");
        }

    } else {
        ESP_LOGE(TAG, "Erro ao obter estado da partição.");
    }

#endif 
}

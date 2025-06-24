#include "ota_manager.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_log.h"
#include "mbedtls/sha256.h"

static const char *TAG = "OTA";

static const esp_partition_t *ota_partition = NULL;
static esp_ota_handle_t ota_handle = 0;
static mbedtls_sha256_context sha_ctx;
static bool ota_in_progress = false;

esp_err_t ota_process_init(size_t img_size) {

    if (ota_in_progress) { // Update already in progress
        return ESP_ERR_INVALID_STATE;
    }

    ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) { // Invalid OTA partition
        return ESP_FAIL; 
    }

    ESP_LOGI(TAG, "Iniciando OTA para partição: %s", ota_partition->label);

    // Allocates memory for the OTA partition
    ESP_ERROR_CHECK(esp_ota_begin(ota_partition, img_size, &ota_handle));

    // Initialize the context and starts the message digest computation
    mbedtls_sha256_init(&sha_ctx);
    mbedtls_sha256_starts(&sha_ctx, 0); // 0 for SHA-256

    ota_in_progress = true;
    return ESP_OK;
}

esp_err_t ota_process_write_block(const uint8_t *data, size_t data_len) {
    if (!ota_in_progress) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_ERROR_CHECK(esp_ota_write(ota_handle, data, data_len));
    // Updates SHA256 computation with buffer data
    mbedtls_sha256_update(&sha_ctx, data, data_len);

    return ESP_OK;
}

esp_err_t ota_process_end(uint8_t *out_sha256) {
    if (!ota_in_progress) {
        return ESP_ERR_INVALID_STATE;
    }

    // Finish OTA update
    esp_err_t err = esp_ota_end(ota_handle);
    
    // Finishes SHA256 computation
    mbedtls_sha256_finish(&sha_ctx, out_sha256);
    // Free memory allocated for the context
    mbedtls_sha256_free(&sha_ctx);
    
    ota_in_progress = false;

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao finalizar OTA: %s", esp_err_to_name(err));
        return err;
    }

    return esp_ota_set_boot_partition(ota_partition);
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

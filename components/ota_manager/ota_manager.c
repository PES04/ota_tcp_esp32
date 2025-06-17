#include <stdio.h>
#include <stdbool.h>
#include "ota_manager.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "esp_log.h"

static const char *TAG = "OTA";

void ota_rollback(void) {
#if defined(CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE)

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;

    if (esp_ota_get_state_partition(running, &state) == ESP_OK) {

        if (state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "Verificação do firmware pendente.");

            if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
                ESP_LOGI(TAG, "Firmware marcado como válido. Rollback cancelado com sucesso.");
            } else {
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

esp_err_t ota_write_firmware(const uint8_t *data, size_t data_len) {

    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    ESP_ERROR_CHECK(esp_ota_begin(update_partition, data_len, &update_handle));
    ESP_ERROR_CHECK(esp_ota_write(update_handle, data, data_len));
    ESP_ERROR_CHECK(esp_ota_end(update_handle));

    return ESP_OK;

}

#include <stdio.h>
#include <stdbool.h>
#include "ota_manager.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"

esp_err_t ota_write_firmware(const uint8_t *data, size_t data_len) {

    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    ESP_ERROR_CHECK(esp_ota_begin(update_partition, data_len, &update_handle));
    ESP_ERROR_CHECK(esp_ota_write(update_handle, data, data_len));
    ESP_ERROR_CHECK(esp_ota_end(update_handle));

    return esp_ota_set_boot_partition(update_partition); // precisa reiniciar o ESP depois

}

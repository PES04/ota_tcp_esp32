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
static types_error_code_e ota_compare_hashes(const uint8_t *recv_hash, const uint8_t *calc_hash);

/**
 * @brief Initializes an Over-The-Air (OTA) update process by setting the firmware size, copying the hash, 
 * selecting the next OTA partition, and preparing the partition for writing. 
 * It also initializes a SHA-256 context for hash computation and returns an appropriate error code 
 * based on the success or failure of the initialization steps.
 * 
 * @param img_size Firmware size to be updated
 * @param hash Received hash
 * @return types_error_code_e
 */
types_error_code_e ota_process_init(const size_t img_size, const uint8_t* hash) {

    if (ota_in_progress) { // Update already in progress
        return ERR_CODE_NOT_ALLOWED;
    }

    fmw_size = img_size;
    memcpy(sent_hash, hash, HASH_SIZE_IN_BYTES);


    ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) { // Invalid OTA partition
        ota_in_progress = false;
        return ERR_CODE_FAIL; 
    }

    ESP_LOGI(TAG, "Initializing OTA to partition: %s", ota_partition->label);

    // Allocates memory for the OTA partition
    ESP_ERROR_CHECK(esp_ota_begin(ota_partition, fmw_size, &ota_handle));

    // Initialize the context and starts the message digest computation
    mbedtls_sha256_init(&sha_ctx);
    mbedtls_sha256_starts(&sha_ctx, 0); // 0 for SHA-256

    ota_in_progress = true;
    return ERR_CODE_OK;
}

/**
 * @brief Writes a block of data to an ongoing Over-The-Air (OTA) update process, verifies the integrity 
 * of the data using SHA-256 hashing, and checks the firmware size against the expected size. 
 * It returns an error code indicating the status of the operation, such as success, failure, 
 * or in-progress, and handles errors like mismatched firmware size or hash computation failures.
 * 
 * @param data Firmware block
 * @param data_len Firmware block size
 * @return types_error_code_e 
 */
types_error_code_e ota_process_write_block(const uint8_t *data, const size_t data_len) {

    if (!ota_in_progress) { 
        return ERR_CODE_NOT_ALLOWED;
    }

    esp_err_t err = esp_ota_write(ota_handle, data, data_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error writing OTA: %s", esp_err_to_name(err));
        ota_in_progress = false;
        updated_fmw_size = 0;
        return ERR_CODE_FAIL;
    }

    // Updates SHA256 computation with buffer data
    mbedtls_sha256_update(&sha_ctx, data, data_len);

    updated_fmw_size += data_len;

    if (updated_fmw_size < fmw_size) {
        return ERR_CODE_IN_PROGRESS;

    } else if (updated_fmw_size > fmw_size) {
        ESP_LOGE(TAG, "Updated firmware size different from received.");
        ota_in_progress = false;
        updated_fmw_size = 0;
        return ERR_CODE_FAIL;

    } else {
        uint8_t calc_hash[HASH_SIZE_IN_BYTES] = {0};

        if (ota_process_compute_hash(calc_hash) != 0) {
            ESP_LOGE(TAG, "Failed to compute hash SHA-256");
            ota_in_progress = false;
            updated_fmw_size = 0;
            return ERR_CODE_FAIL;
        }

        return (ota_compare_hashes(sent_hash, calc_hash));     
    }
}

/**
 * @brief Computes and finalizes a SHA-256 hash, storing the result in the buffer pointed to by out_sha256. 
 * It uses the mbedtls_sha256_finish function to complete the hash computation.
 * 
 * @param out_sha256 Output parameter of hash
 * @return int, 0 on success and negative value on failure
 */
int ota_process_compute_hash(uint8_t *out_sha256) {
    
    // Finishes SHA256 computation
    return mbedtls_sha256_finish(&sha_ctx, out_sha256);

}

/**
 * @brief Concludes an ongoing OTA (Over-The-Air) update process, ensuring proper cleanup and validation. 
 * It checks if the system is healthy, frees allocated resources, finalizes the OTA update, 
 * sets the new boot partition, and returns an appropriate error code based on the operation's success or failure.
 * 
 * @param is_healthy true when writing process was sucessful
 * @return types_error_code_e 
 */
types_error_code_e ota_process_end(bool is_healthy) {

    if (!ota_in_progress) { 
        return ERR_CODE_NOT_ALLOWED;
    }

    ota_in_progress = false;
    updated_fmw_size = 0;
    // Free memory allocated for the context
    mbedtls_sha256_free(&sha_ctx);

    if (!is_healthy) {
        ESP_LOGE(TAG, "OTA update interrupted: system not healthy.");
        return ERR_CODE_FAIL;
    }

    // Finish OTA update
    ESP_ERROR_CHECK(esp_ota_end(ota_handle));

    if (esp_ota_set_boot_partition(ota_partition) != ESP_OK) {
        ESP_LOGE(TAG, "Error setting up new OTA partition");
        return ERR_CODE_FAIL;
    }

    return ERR_CODE_OK;
}

/**
 * @brief Compares two hash values, recv_hash and calc_hash, byte by byte to verify their equality. 
 * If any mismatch is found, it logs an error, updates OTA-related state variables, and returns ERR_CODE_FAIL; 
 * otherwise, it returns ERR_CODE_OK.
 * 
 * @param recv_hash Received hash
 * @param calc_hash Calculated hash
 * @return types_error_code_e 
 */
static types_error_code_e ota_compare_hashes(const uint8_t *recv_hash, const uint8_t *calc_hash) {

    for (int i = 0; i < HASH_SIZE_IN_BYTES; i++) {
        if (recv_hash[i] != calc_hash[i]) {
            ESP_LOGE(TAG, "Different hashes.");
            ota_in_progress = false;
            updated_fmw_size = 0;
            return ERR_CODE_FAIL;
        }
    }

    return ERR_CODE_OK;
}

/**
 * @brief Evaluates the health of the system and manages OTA rollback behavior based on the firmware state. 
 * If the system is unhealthy or the firmware verification fails, it triggers a rollback and reboot; 
 * otherwise, it marks the firmware as valid and cancels the rollback.
 * 
 * @param is_healthy true when system is healthy, false otherwise
 */
void ota_check_rollback(bool is_healthy) {
#if defined(CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE)

    if (!is_healthy) {
        ESP_LOGE(TAG, "OTA update cancelled: system not healthy.");
        esp_ota_mark_app_invalid_rollback_and_reboot();
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;

    if (esp_ota_get_state_partition(running, &state) == ESP_OK) {

        if (state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "Pending firmware verification.");

            if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) { // Cancels rollback
                ESP_LOGI(TAG, "Firmware marked as valid. Rollback cancelled successfully.");
                
            } else { // Rolls back to the previously workable app and restarts ESP
                ESP_LOGE(TAG, "Failed to mark firmware as valid.");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }

        } else {
            ESP_LOGI(TAG, "----- Firmware already valid -----");
        }

    } else {
        ESP_LOGE(TAG, "Error to get partition state.");
        esp_ota_mark_app_invalid_rollback_and_reboot();
    }

#endif 
}

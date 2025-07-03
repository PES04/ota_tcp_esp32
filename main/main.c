#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sys_initializer.h"
#include "wifi_ap.h"
#include "tcp_tls.h"
#include "ota_manager.h"
#include "sys_feedback.h"


#define VERSION_MAJOR       (1U)
#define VERSION_MINOR       (0U)
#define VERSION_PATCH       (0U)

#define RESET_DELAY_MS      (1000)


static const char *tag = "MAIN";


static void init_err(void);

/**
 * @brief Main task
 * 
 */
void app_main(void)
{
    ESP_LOGI(tag, "----- Starting application -----");
    
    sys_feedback_whoiam(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    
    if (sys_feedback_init() != ERR_CODE_OK)
    {
        init_err();
    }
    else
    {
        ESP_LOGI(tag, "----- sys_feedback: OK -----");
    }

    if (sys_initializer_init() != ERR_CODE_OK)
    {
        init_err();
    }
    else
    {
        ESP_LOGI(tag, "----- sys_initializer: OK -----");
    }
    
    wifi_ap_init();
    ESP_LOGI(tag, "----- wifi_ap: OK -----");
    
    if (tcp_tls_init() != ERR_CODE_OK)
    {
        init_err();
    }
    else
    {
        ESP_LOGI(tag, "----- tcp_tls: OK -----");
    }

    ota_check_rollback(true);
}

/**
 * @brief Invalid the current firmware and reset the system
 * 
 */
static void init_err(void)
{
    while (1)
    {
        ESP_LOGE(tag, "----- Error initializing the system -----");
        ota_check_rollback(false);
        vTaskDelay(pdMS_TO_TICKS(RESET_DELAY_MS));
    }
}

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "sys_initializer.h"
#include "wifi_ap.h"
#include "tcp_tls.h"
#include "ota_manager.h"

static const char *tag = "MAIN";


static void init_err(void);


/**
 * @brief Main task
 * 
 */
void app_main(void)
{
    ESP_LOGI(tag, "----- Starting application -----");
    
    if (sys_initializer_init() != ERR_CODE_OK)
    {
        
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

static void init_err(void)
{
    while (1)
    {
        ESP_LOGE(tag, "----- Error initializing the system -----");
        vTaskDelay(pdMS_TO_TICKS(10000));
        ota_check_rollback(false);
    }
}

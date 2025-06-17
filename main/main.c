#include <stdio.h>

#include "esp_log.h"
#include "sys_initializer.h"
#include "wifi_ap.h"
#include "tcp_tls.h"

static const char *tag = "MAIN";

/**
 * @brief Main task
 * 
 */
void app_main(void)
{
    ESP_LOGI(tag, "----- Starting application -----");
    
    tcp_tls_init();
    
    sys_initializer_init();
    
    wifi_ap_init();
}

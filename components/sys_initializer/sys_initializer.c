#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "tcp_tls.h"
#include "wifi_ap.h"
#include "sys_initializer.h"


static const char *tag = "SYS_INITIALIZER";


static void sys_initializer_wifi(void);
static void sys_initializer_tcp_tls(void);


void sys_initializer_init(void)
{
    ESP_LOGI(tag, "----- Initializing NVS -----");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(tag, "----- NVS initialized -----");
    
    sys_initializer_wifi();
    sys_initializer_tcp_tls();
}

static void sys_initializer_wifi(void)
{
    char ssid[WIFI_AP_SSID_MAX_LEN] = {};
    char password[WIFI_AP_PASSWORD_MAX_LEN] = {};

    nvs_handle_t nvs_handle = 0;
    esp_err_t ret = nvs_open("wifi_ap_config", NVS_READONLY, &nvs_handle);

    if (ret != ESP_OK)
    {
        ESP_LOGE(tag, "----- Can not open wifi_ap_config namespace -----");
    }
    else
    {
        /* SSID initialization */
        size_t ssid_len = sizeof(ssid);
        ret = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_len);

        if (ret != ESP_OK)
        {
            ESP_LOGW(tag, "----- Can not read ssid -----");
        }
        else
        {
            wifi_ap_set_ssid(ssid, strlen(ssid));
        }

        /* Password initialization */
        size_t password_len = sizeof(password);
        ret = nvs_get_str(nvs_handle, "password", password, &password_len);

        if (ret != ESP_OK)
        {
            ESP_LOGW(tag, "----- Can not read password -----");
        }
        else
        {
            wifi_ap_set_password(password, strlen(password));
        }

        nvs_close(nvs_handle);
    }
}

static void sys_initializer_tcp_tls(void)
{
    nvs_handle_t nvs_handle = 0;
    esp_err_t ret = nvs_open("tls_config", NVS_READONLY, &nvs_handle);

    uint8_t buffer[TCP_TLS_MAX_BUFFER_LEN] = {};

    if (ret != ESP_OK)
    {
        ESP_LOGE(tag, "----- Can not open tls_config namespace -----");
    }
    else
    {
        /* Server certificate initialization */
        size_t buffer_len = sizeof(buffer);
        ret = nvs_get_blob(nvs_handle, "server_crt", buffer, &buffer_len);
        if (ret != ESP_OK)
        {
            ESP_LOGE(tag, "----- Can not read server certificate -----");
        }
        else
        {
         tcp_tls_set_server_ctr(buffer, buffer_len);
        }

        /* Server key initialization */
        buffer_len = sizeof(buffer);
        ret = nvs_get_blob(nvs_handle, "server_key", buffer, &buffer_len);
        if (ret != ESP_OK)
        {
            ESP_LOGE(tag, "----- Can not read server key -----");
        }
        else
        {
         tcp_tls_set_server_key(buffer, buffer_len);
        }

        nvs_close(nvs_handle);
    }
}

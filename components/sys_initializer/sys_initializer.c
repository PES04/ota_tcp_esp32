#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"
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
    
    sys_initializer_wifi();
    sys_initializer_tcp_tls();
    
    ESP_LOGI(tag, "----- NVS Initialized -----");
}

static void sys_initializer_wifi(void)
{
    char ssid[WIFI_AP_SSID_MAX_LEN] = {};
    char password[WIFI_AP_PASSWORD_MAX_LEN] = {};

    nvs_handle_t nvs_handle = 0;
    esp_err_t ret = nvs_open("wifi_ap_config", NVS_READONLY, &nvs_handle);

    if (ret != ESP_OK)
    {
        ESP_LOGE(tag, "----- Can not open NVS Namespace -----");
    }
    else
    {
        /* SSID Initialization */
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

        /* Password Initialization */
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

    char buffer[4198] = {};

    if (ret != ESP_OK)
    {
        ESP_LOGE(tag, "----- Can not open NVS Namespace -----");
    }
    else
    {
        // size_t cert_len = 0;
        // esp_err_t err = nvs_get_str(nvs_handle, "server_cert", NULL, &cert_len);

        // if (err != ESP_OK) {
        //     ESP_LOGE(tag, "Erro ao pegar tamanho do certificado: %s", esp_err_to_name(err));
        //     return;
        // }

        size_t cert_len = sizeof(buffer);
        ret = nvs_get_str(nvs_handle, "server_cert", buffer, &cert_len);
        if (ret != ESP_OK)
        {
            ESP_LOGW(tag, "----- Can not read certificate -----");
        }
        else
        {
         ESP_LOGI(tag, "Certificado lido:\n%s", buffer);
        }

        nvs_close(nvs_handle);
    }
}
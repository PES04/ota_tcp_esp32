#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "tcp_tls.h"
#include "wifi_ap.h"
#include "sys_initializer.h"


static const char *tag = "SYS_INITIALIZER";


static esp_err_t init_wifi_params(void);
static esp_err_t init_tcp_tls_params(void);

/**
 * @brief Initialize the sys_initializer component
 * 
 */
esp_err_t sys_initializer_init(void)
{
    ESP_LOGI(tag, "----- Initializing NVS -----");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(tag, "----- NVS initialized -----");
    
    ret = init_wifi_params();
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = init_tcp_tls_params();

    return ret;
}

/**
 * @brief Initialize the wifi parameters
 * 
 */
static esp_err_t init_wifi_params(void)
{
    char wifi_config[WIFI_AP_SSID_MAX_LEN + WIFI_AP_PASSWORD_MAX_LEN] = {};

    /* Open wifi parameters namespace */
    nvs_handle_t nvs_handle = 0;
    esp_err_t ret = nvs_open("wifi_ap_config", NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(tag, "----- Can not open wifi_ap_config namespace -----");
        return ret;
    }

    /* Wifi parameters initialization */
    size_t wifi_config_len = sizeof(wifi_config);
    ret = nvs_get_blob(nvs_handle, "wifi_params", wifi_config, &wifi_config_len);
    if (ret != ESP_OK)
    {
        ESP_LOGW(tag, "----- Can not read wifi parameters -----");
        return ret;
    }

    /* Wifi parameters parsing (ssid;password) */
    /* SSID Parsing */
    char *token = strtok(wifi_config, ";");
    if (token == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    else
    {
        ret = wifi_ap_set_ssid(token, strlen(token));
        if (ret != ESP_OK)
        {
            return ret;
        }
    }

    /* Password Parsing */
    token = strtok(NULL, ";");
    if (token == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    else
    {
        ret = wifi_ap_set_password(token, strlen(token));
        if (ret != ESP_OK)
        {
            return ret;
        }
    }

    nvs_close(nvs_handle);

    return ret;
}

/**
 * @brief Initialize the TLS connection asssets
 * 
 */
static esp_err_t init_tcp_tls_params(void)
{
    uint8_t buffer[TCP_TLS_MAX_BUFFER_LEN] = {};
    
    nvs_handle_t nvs_handle = 0;
    esp_err_t ret = nvs_open("tls_config", NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(tag, "----- Can not open tls_config namespace -----");
        return ret;
    }

    /* Server certificate initialization */
    size_t buffer_len = sizeof(buffer);
    ret = nvs_get_blob(nvs_handle, "server_crt", buffer, &buffer_len);
    if (ret != ESP_OK)
    {
        ESP_LOGE(tag, "----- Can not read server certificate -----");
        return ret;
    }

    ret = tcp_tls_set_server_crt(buffer, buffer_len);
    if (ret != ESP_OK)
    {
        return ret;
    }

    /* Server key initialization */
    buffer_len = sizeof(buffer);
    ret = nvs_get_blob(nvs_handle, "server_key", buffer, &buffer_len);
    if (ret != ESP_OK)
    {
        ESP_LOGE(tag, "----- Can not read server key -----");
        return ret;
    }
        
    ret = tcp_tls_set_server_key(buffer, buffer_len);

    nvs_close(nvs_handle);

    return ret;
}

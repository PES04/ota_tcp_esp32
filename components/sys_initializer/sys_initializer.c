#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "tcp_tls.h"
#include "wifi_ap.h"
#include "sys_initializer.h"


static const char *tag = "SYS_INITIALIZER";


static types_error_code_e init_wifi_params(void);
static types_error_code_e init_tcp_tls_params(void);

/**
 * @brief Initialize the sys_initializer component
 * 
 */
types_error_code_e sys_initializer_init(void)
{
    ESP_LOGI(tag, "----- Initializing NVS -----");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(tag, "----- NVS initialized -----");
    
    types_error_code_e err = init_wifi_params();
    if (err != ERR_CODE_OK)
    {
        return ret;
    }

    err = init_tcp_tls_params();

    return err;
}

/**
 * @brief Initialize the wifi parameters
 * 
 */
static types_error_code_e init_wifi_params(void)
{
    char wifi_config[WIFI_AP_SSID_MAX_LEN + WIFI_AP_PASSWORD_MAX_LEN] = {};

    /* Open wifi parameters namespace */
    nvs_handle_t nvs_handle = 0;
    ESP_ERROR_CHECK(nvs_open("wifi_ap_config", NVS_READONLY, &nvs_handle));

    /* Wifi parameters initialization */
    size_t wifi_config_len = sizeof(wifi_config);
    ESP_ERROR_CHECK(nvs_get_blob(nvs_handle, "wifi_params", wifi_config, &wifi_config_len));

    types_error_code_e err = ERR_CODE_OK;
    /* Wifi parameters parsing (ssid;password) */
    /* SSID Parsing */
    char *token = strtok(wifi_config, ";");
    if (token == NULL)
    {
        return ERR_CODE_INVALID_PARAM;
    }
    else
    {
        err = wifi_ap_set_ssid(token, strlen(token));
        if (err != ERR_CODE_OK)
        {
            return err;
        }
    }

    /* SSID Parsing */
    token = strtok(NULL, ";");
    if (token == NULL)
    {
        return ERR_CODE_INVALID_PARAM;
    }
    else
    {
        err = wifi_ap_set_password(token, strlen(token));
        if (err != ERR_CODE_OK)
        {
            return err;
        }
    }

    nvs_close(nvs_handle);

    return err;
}

/**
 * @brief Initialize the TLS connection asssets
 * 
 */
static types_error_code_e init_tcp_tls_params(void)
{
    uint8_t buffer[TCP_TLS_MAX_BUFFER_LEN] = {};
    
    nvs_handle_t nvs_handle = 0;
    ESP_ERROR_CHECK(nvs_open("tls_config", NVS_READONLY, &nvs_handle));

    /* Server certificate initialization */
    size_t buffer_len = sizeof(buffer);
    ESP_ERROR_CHECK(nvs_get_blob(nvs_handle, "server_crt", buffer, &buffer_len));

    types_error_code_e err = tcp_tls_set_server_crt(buffer, buffer_len);
    if (err != ERR_CODE_OK)
    {
        return err;
    }

    /* Server key initialization */
    buffer_len = sizeof(buffer);
    ESP_ERROR_CHECK(nvs_get_blob(nvs_handle, "server_key", buffer, &buffer_len));
        
    err = tcp_tls_set_server_key(buffer, buffer_len);

    nvs_close(nvs_handle);

    return err;
}

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_mac.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi_ap.h"


#define PASSWORD_MIN_LEN                    (8U) /* Min length of Wi-Fi API. Don't choose less than 8 bytes */
#define MAX_CLIENTS                         (1U)
#define WIFI_CHANNEL                        (1U)


static const char *tag = "WIFI_AP";

static char wifi_ap_ssid[WIFI_AP_SSID_MAX_LEN] = {};
static char wifi_ap_password[WIFI_AP_PASSWORD_MAX_LEN] = {};

/* ---------------------------- Private Function ---------------------------- */
static void wifi_init_softap(void);
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
/* -------------------------------------------------------------------------- */


/**
 * @brief Initialize wifi_ap component
 * 
 */
void wifi_ap_init(void)
{    
    wifi_init_softap();
}

/**
 * @brief SSID setter
 * 
 * @param ssid [in]: Wi-Fi AP SSID
 * @param len [in]: SSID length
 */
esp_err_t wifi_ap_set_ssid(char *ssid, const uint8_t len)
{
    static bool has_ssid_set = false;
    
    if (has_ssid_set)
    {
        ESP_LOGE(tag, "----- Wi-Fi SSID already set -----");
        return ESP_ERR_NOT_ALLOWED;
    }

    if ((len >= WIFI_AP_SSID_MAX_LEN) || (len <= 0U))
    {
        ESP_LOGE(tag, "----- Wi-Fi SSID out of valid range -----");
        return ESP_ERR_INVALID_ARG;
    }
    
    strncpy(wifi_ap_ssid, ssid, len);
    wifi_ap_ssid[len] = '\0';

    has_ssid_set = true;

    ESP_LOGI(tag, "----- Wi-Fi SSID has set -----");

    return ESP_OK;
}

/**
 * @brief Password setter
 * 
 * @param password [in]: Wi-Fi AP password
 * @param len [in]: Password length
 */
esp_err_t wifi_ap_set_password(char *password, const uint8_t len)
{
    static bool has_password_set = false;
    
    if (has_password_set)
    {
        ESP_LOGE(tag, "----- Wi-Fi password already set -----");
        return ESP_ERR_NOT_ALLOWED;
    }
    
    if ((len >= WIFI_AP_PASSWORD_MAX_LEN) || (len < PASSWORD_MIN_LEN))
    {
        ESP_LOGE(tag, "----- Wi-Fi password out of valid range -----");
        return ESP_ERR_INVALID_ARG;
    }
    
    strncpy(wifi_ap_password, password, len);
    wifi_ap_password[len] = '\0';

    has_password_set = true;

    ESP_LOGI(tag, "----- Wi-Fi password has set -----");

    return ESP_OK;
}

/**
 * @brief ESP-IDF Wi-Fi event handler 
 * 
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) 
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(tag, "----- Station "MACSTR" join, AID=%d -----",
                 MAC2STR(event->mac), event->aid);
    } 
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) 
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(tag, "----- Station "MACSTR" leave, AID=%d, reason=%d -----",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}

/**
 * @brief Initialize Wi-Fi in AP configuration
 * 
 */
static void wifi_init_softap(void)
{
    ESP_LOGI(tag, "----- Initializing Wi-Fi in AP Mode -----");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(wifi_ap_ssid),
            .channel = WIFI_CHANNEL,
            .max_connection = MAX_CLIENTS,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                    .required = true,
            },
        },
    };

    memcpy(wifi_config.ap.ssid, wifi_ap_ssid, sizeof(wifi_config.ap.ssid));
    memcpy(wifi_config.ap.password, wifi_ap_password, sizeof(wifi_config.ap.password));

    if (strlen(wifi_ap_password) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(tag, "----- Wi-Fi AP Mode Initialized -----");
}


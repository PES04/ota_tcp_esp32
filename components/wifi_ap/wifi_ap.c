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


#define MAX_CLIENTS         (1U)
#define WIFI_CHANNEL        (1U)


static const char *tag = "WIFI_AP";

static char wifi_ap_ssid[WIFI_AP_SSID_MAX_LEN] = "Default";
static char wifi_ap_password[WIFI_AP_PASSWORD_MAX_LEN] = "default123";


/* ---------------------------- Private Function ---------------------------- */
static void wifi_init_softap(void);
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
/* -------------------------------------------------------------------------- */


void wifi_ap_init(void)
{
    ESP_LOGI(tag, "----- Initializing Wi-Fi in AP Mode -----");
    wifi_init_softap();
}

void wifi_ap_set_ssid(char *ssid, const uint8_t len)
{
    strncpy(wifi_ap_ssid, ssid, len);
    wifi_ap_ssid[len] = '\0';

    ESP_LOGI(tag, "----- SSID HAS SET -----");
}

void wifi_ap_set_password(char *password, const uint8_t len)
{
    strncpy(wifi_ap_password, password, len);
    wifi_ap_password[len] = '\0';

    ESP_LOGI(tag, "----- PASSWORD HAS SET -----");
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) 
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(tag, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } 
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) 
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(tag, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}

static void wifi_init_softap(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifi_event_handler,
                                        NULL,
                                        NULL);

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

    if (strlen(wifi_ap_password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(tag, "----- Wi-Fi AP Mode Initialized -----");
}


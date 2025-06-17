#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "esp_tls.h"
#include "tcp_tls.h"
#include "ota_manager.h"

#define COUNT_NEEDED_TO_START_TCP_SOCKET      (2U)


typedef struct {
    uint8_t val[TCP_TLS_MAX_BUFFER_LEN];
    size_t len;
} crypt_buffer_t;


static const char *tag = "TCP_TLS";

static crypt_buffer_t server_crt = {};
static crypt_buffer_t server_key = {};
static bool has_server_crt_set = false;
static bool has_server_key_set = false;
static SemaphoreHandle_t semphr_sync = NULL;

/* ------------------- Private Functions ------------------- */
static void tcp_tls_task(void * params);
/* --------------------------------------------------------- */

void tcp_tls_init(void)
{
    semphr_sync = xSemaphoreCreateCounting(COUNT_NEEDED_TO_START_TCP_SOCKET, 0);
    if (semphr_sync == NULL)
    {
        ESP_LOGE(tag, "----- Error creating sync semaphore -----");
        return;
    }

    ESP_LOGI(tag, "----- Initializing tcp_tls task -----");
    xTaskCreate(tcp_tls_task, "tcp_tls_task", 4096, NULL, 4, NULL);
}

void tcp_tls_set_server_ctr(const uint8_t *crt, const size_t len)
{
    if ((has_server_crt_set == true) || (len > TCP_TLS_MAX_BUFFER_LEN))
    {
        return;
    }

    memcpy(server_crt.val, crt, len);
    server_crt.len = len;

    has_server_crt_set = true;
    xSemaphoreGive(semphr_sync);

    ESP_LOGI(tag, "----- Server certificate has set -----");
}

void tcp_tls_set_server_key(const uint8_t *key, const size_t len)
{
    if ((has_server_key_set == true) || (len > TCP_TLS_MAX_BUFFER_LEN))
    {
        return;
    }

    memcpy(server_key.val, key, len);
    server_key.len = len;

    has_server_key_set = true;
    xSemaphoreGive(semphr_sync);

    ESP_LOGI(tag, "----- Server key has set -----");
}

static void tcp_tls_task(void * params)
{
    while (uxSemaphoreGetCount(semphr_sync) != COUNT_NEEDED_TO_START_TCP_SOCKET)
    {
        vTaskDelay(pdMS_TO_TICKS(250));
    }
    
    ESP_LOGI(tag, "----- Creating socket -----");
    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    struct sockaddr_in dest_addr = {
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_family = AF_INET,
        .sin_port = htons(2000)
    };

    esp_tls_cfg_server_t server_cfg = {
        .servercert_buf = server_crt.val,
        .servercert_bytes = server_crt.len,
        .serverkey_buf = server_key.val,
        .serverkey_bytes = server_key.len
    };

    ESP_LOGI(tag, "----- Binding socket -----");
    int ret = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (ret != 0)
    {
        ESP_LOGE(tag, "----- Can not bind the socket -----");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(tag, "----- Listening socket -----");
    ret = listen(listen_sock, 1);
    if (ret != 0)
    {
        ESP_LOGE(tag, "----- Can not listen the socket -----");
        vTaskDelete(NULL);
        return;
    }

    while (1)
    {
        struct sockaddr_storage source_addr = {};
        socklen_t addr_len = sizeof(source_addr);

        ESP_LOGI(tag, "----- Wainting for a connection -----");
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0)
        {
            ESP_LOGE(tag, "----- Unable to accept the connection -----");
            continue;;
        }

        uint8_t rx_buffer[64] = {};

        while (1)
        {
            int32_t len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len < 0)
            {
                ESP_LOGE(tag, "----- Receving error -----");
                break;
            }
            else if (len == 0)
            {
                ESP_LOGW(tag, "----- Client disconnected -----");
                break;
            }
            else
            {
                rx_buffer[len] = '\0';
                printf("Recebido %ld bytes: \"%s\"\n", len, (char *)rx_buffer);

                // Write OTA
                if (ota_write_firmware(rx_buffer, len) == ESP_OK) {
                    ESP_LOGI(tag, "ESP Firmware written successfully");
                    vTaskDelay(pdMS_TO_TICKS(500));
                    esp_restart();
                } else {
                    ESP_LOGE(tag, "ESP Firmware failed to update");
                }
            }
        }

        ESP_LOGI(tag, "----- Closing socket -----");

        close(sock);
    }

    ESP_LOGI(tag, "----- Closing listening socket -----");

    close(listen_sock);
    vTaskDelete(NULL);
}

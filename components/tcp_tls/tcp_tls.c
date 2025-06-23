#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "esp_tls.h"
#include "msg_parser.h"
#include "tcp_tls.h"


#define COUNT_NEEDED_TO_START_TCP_SOCKET      (2U)


typedef struct {
    uint8_t val[TCP_TLS_MAX_BUFFER_LEN];
    size_t len;
} crypt_buffer_t;


static const char *tag = "TCP_TLS";

static crypt_buffer_t server_crt = {};
static crypt_buffer_t server_key = {};

/* ------------------- Private Functions ------------------- */
static void tcp_tls_task(void * params);
/* --------------------------------------------------------- */

/**
 * @brief Initialize the tcp_tls component
 * 
 */
void tcp_tls_init(void)
{
    ESP_LOGI(tag, "----- Initializing tcp_tls task -----");
    xTaskCreate(tcp_tls_task, "tcp_tls_task", 4096, NULL, 4, NULL);
}

/**
 * @brief Server_crt setter
 * 
 * @param crt [in]: Server certificate
 * @param len [in]: Server certificate length in bytes
 */
esp_err_t tcp_tls_set_server_crt(const uint8_t *crt, const size_t len)
{
    static bool has_server_crt_set = false;
    
    if (has_server_crt_set == true)
    {
        ESP_LOGE(tag, "----- Server certificate already set -----");
        return ESP_ERR_NOT_ALLOWED;
    }

    if (len >= TCP_TLS_MAX_BUFFER_LEN)
    {
        ESP_LOGE(tag, "----- Server certificate invalid range -----");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(server_crt.val, crt, len);
    server_crt.val[len] = '\0'; /* Needed to mbedtls */
    server_crt.len = len + 1U;

    has_server_crt_set = true;

    ESP_LOGI(tag, "----- Server certificate has set -----");

    return ESP_OK;
}

/**
 * @brief Server_key setter
 * 
 * @param key [in]: Server key
 * @param len [in]: Server key length in bytes
 */
esp_err_t tcp_tls_set_server_key(const uint8_t *key, const size_t len)
{
    static bool has_server_key_set = false;
    
    if (has_server_key_set == true)
    {
        ESP_LOGE(tag, "----- Server key already set -----");
        return ESP_ERR_NOT_ALLOWED;
    }
    
    if (len >= TCP_TLS_MAX_BUFFER_LEN)
    {
        ESP_LOGE(tag, "----- Server key invalid range -----");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(server_key.val, key, len);
    server_key.val[len] = '\0'; /* Needed to mbedtls */
    server_key.len = len + 1U;

    has_server_key_set = true;

    ESP_LOGI(tag, "----- Server key has set -----");

    return ESP_OK;
}

/**
 * @brief TLS main task
 * 
 * @param params [in]: Task parameters
 */
static void tcp_tls_task(void * params)
{
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

    ESP_LOGI(tag, "----- Creating listening socket -----");
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
            continue;
        }

        esp_tls_t *tls = esp_tls_init();
        if (tls == NULL)
        {
            ESP_LOGE(tag, "----- Unable to create TLS section -----");
            close(sock);
            continue;
        }

        if (esp_tls_server_session_create(&server_cfg, sock, tls) != 0)
        {
            ESP_LOGE(tag, "----- Unable to establish TLS connection -----");
            esp_tls_conn_destroy(tls);
            continue;
        }

        uint8_t rx_buffer[64] = {};

        while (1)
        {
            int32_t len = esp_tls_conn_read(tls, rx_buffer, sizeof(rx_buffer) - 1);
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

                msg_parser_run(rx_buffer, len);
            }
        }

        ESP_LOGI(tag, "----- Closing socket -----");

        esp_tls_conn_destroy(tls);
    }

    ESP_LOGI(tag, "----- Closing listening socket -----");

    close(listen_sock);
    vTaskDelete(NULL);
}

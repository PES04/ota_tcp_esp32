#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "esp_tls.h"
#include "msg_parser.h"
#include "tcp_tls.h"


#define COUNT_NEEDED_TO_START_TCP_SOCKET        (2U)
#define TCP_BUFFER_LEN_BYTES                    (2048U)

#define KEEPIDLE_TIME_SEC                       (30)
#define KEEPINTERVAL_SEC                        (5)
#define KEEPCOUNT                               (2)

#define RX_TIMEOUT_SEC                          (10)
#define RX_TIMEOUT_USEC                         (0)


typedef struct {
    uint8_t val[TCP_TLS_MAX_BUFFER_LEN];
    size_t len;
} crypt_buffer_t;


static const char *tag = "TCP_TLS";

static crypt_buffer_t server_crt = {};
static crypt_buffer_t server_key = {};

/* ------------------- Private Functions ------------------- */
static void tcp_tls_task(void * params);
static types_error_code_e run_conn_rx(esp_tls_t *tls, const uint8_t * rx_buffer, const int32_t rx_len);
/* --------------------------------------------------------- */

/**
 * @brief Initialize the tcp_tls component
 * 
 */
types_error_code_e tcp_tls_init(void)
{
    ESP_LOGI(tag, "----- Initializing tcp_tls task -----");
    xTaskCreate(tcp_tls_task, "tcp_tls_task", 8192, NULL, 4, NULL);

    types_error_code_e err = msg_parser_init();

    return err;
}

/**
 * @brief Server_crt setter
 * 
 * @param crt [in]: Server certificate
 * @param len [in]: Server certificate length in bytes
 */
types_error_code_e tcp_tls_set_server_crt(const uint8_t *crt, const size_t len)
{
    static bool has_server_crt_set = false;
    
    if (has_server_crt_set == true)
    {
        ESP_LOGE(tag, "----- Server certificate already set -----");
        return ERR_CODE_NOT_ALLOWED;
    }

    if (len >= TCP_TLS_MAX_BUFFER_LEN)
    {
        ESP_LOGE(tag, "----- Server certificate invalid range -----");
        return ERR_CODE_INVALID_PARAM;
    }

    memcpy(server_crt.val, crt, len);
    server_crt.val[len] = '\0'; /* Needed to mbedtls */
    server_crt.len = len + 1U;

    has_server_crt_set = true;

    ESP_LOGI(tag, "----- Server certificate has set -----");

    return ERR_CODE_OK;
}

/**
 * @brief Server_key setter
 * 
 * @param key [in]: Server key
 * @param len [in]: Server key length in bytes
 */
types_error_code_e tcp_tls_set_server_key(const uint8_t *key, const size_t len)
{
    static bool has_server_key_set = false;
    
    if (has_server_key_set == true)
    {
        ESP_LOGE(tag, "----- Server key already set -----");
        return ERR_CODE_NOT_ALLOWED;
    }
    
    if (len >= TCP_TLS_MAX_BUFFER_LEN)
    {
        ESP_LOGE(tag, "----- Server key invalid range -----");
        return ERR_CODE_INVALID_PARAM;
    }

    memcpy(server_key.val, key, len);
    server_key.val[len] = '\0'; /* Needed to mbedtls */
    server_key.len = len + 1U;

    has_server_key_set = true;

    ESP_LOGI(tag, "----- Server key has set -----");

    return ERR_CODE_OK;
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

    int keepAlive = 1;
    int keepIdle = KEEPIDLE_TIME_SEC;
    int keepInterval = KEEPINTERVAL_SEC;
    int keepCount = KEEPCOUNT;

    struct timeval rx_timeout = {
        .tv_sec = RX_TIMEOUT_SEC,
        .tv_usec = RX_TIMEOUT_USEC
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

        /* Keep alive settings */
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        /* Receive timeout settings */
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &rx_timeout, sizeof(rx_timeout));

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

        uint8_t rx_buffer[TCP_BUFFER_LEN_BYTES] = {};

        while (1)
        {
            int32_t rx_len = esp_tls_conn_read(tls, rx_buffer, sizeof(rx_buffer));
            if (rx_len < 0)
            {
                ESP_LOGE(tag, "----- Receving error -----");
                break;
            }
            else if (rx_len == 0)
            {
                ESP_LOGW(tag, "----- Client disconnected -----");
                break;
            }
            else
            {
                if (run_conn_rx(tls, rx_buffer, rx_len) != ERR_CODE_OK)
                {
                    ESP_LOGE(tag, "----- Sending error -----");
                    break;
                }
            }
        }

        msg_parser_clean();
        
        ESP_LOGI(tag, "----- Closing socket -----");

        esp_tls_conn_destroy(tls);
    }

    ESP_LOGI(tag, "----- Closing listening socket -----");

    close(listen_sock);
    vTaskDelete(NULL);
}

static types_error_code_e run_conn_rx(esp_tls_t *tls, const uint8_t * rx_buffer, const int32_t rx_len)
{
    uint32_t firmware_bytes_read = 0;
    types_error_code_e err = msg_parser_run(rx_buffer, rx_len, &firmware_bytes_read);
    
    uint8_t tx_buffer[MSG_PARSER_BUF_LEN_BYTES] = {};
    uint8_t tx_len = 0;

    msg_parser_build_firmware_ack(tx_buffer, sizeof(tx_buffer), &tx_len);

    if (esp_tls_conn_write(tls, tx_buffer, tx_len) < 0)
    {
        return ERR_CODE_INVALID_OP;
    }

    if ((err == ERR_CODE_OK) || (err == ERR_CODE_FAIL))
    {
        msg_parser_build_ota_ack(tx_buffer, sizeof(tx_buffer), (err == ERR_CODE_OK), firmware_bytes_read, &tx_len);

        if (esp_tls_conn_write(tls, tx_buffer, tx_len) < 0)
        {
            return ERR_CODE_INVALID_OP;
        }
    }

    return ERR_CODE_OK;
}

#ifndef TCP_TLS
#define TCP_TLS

#define TCP_TLS_MAX_BUFFER_LEN      (4198U)


void tcp_tls_init(void);

void tcp_tls_set_server_ctr(const uint8_t *crt, const size_t len);

void tcp_tls_set_server_key(const uint8_t *key, const size_t len);

#endif
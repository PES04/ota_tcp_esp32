#ifndef TCP_TLS
#define TCP_TLS

#define TCP_TLS_MAX_BUFFER_LEN      (4198U)

#include "types.h"

void tcp_tls_init(void);

types_error_code_e tcp_tls_set_server_crt(const uint8_t *crt, const size_t len);

types_error_code_e tcp_tls_set_server_key(const uint8_t *key, const size_t len);

#endif
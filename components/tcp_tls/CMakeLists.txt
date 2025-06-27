idf_component_register(SRCS "tcp_tls.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp-tls msg_parser auth_hmac
                    REQUIRES types)

idf_component_register(SRCS "sys_initializer.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES nvs_flash wifi_ap tcp_tls)

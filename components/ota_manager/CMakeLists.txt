idf_component_register(SRCS "ota_manager.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES app_update esp-tls 
                    REQUIRES types)

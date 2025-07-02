idf_component_register(SRCS "sys_feedback.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_driver_gpio
                    REQUIRES types)


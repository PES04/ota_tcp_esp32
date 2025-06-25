idf_component_register(SRCS "msg_parser.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES
                    REQUIRES types ota_manager)
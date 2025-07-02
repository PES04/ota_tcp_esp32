idf_component_register(SRCS "msg_parser.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES ota_manager sys_feedback 
                    REQUIRES types)
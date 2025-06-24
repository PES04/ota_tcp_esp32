#include <stdio.h>

#include "sys_initializer.h"
#include "wifi_ap.h"
#include "tcp_tls.h"
#include "ota_manager.h"

static const char *tag = "MAIN";


void app_main(void)
{
    ota_check_rollback();

    tcp_tls_init();
    
    sys_initializer_init();
    
    wifi_ap_init();
}

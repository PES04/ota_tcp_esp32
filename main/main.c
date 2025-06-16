#include <stdio.h>

#include "sys_initializer.h"
#include "wifi_ap.h"
#include "tcp_tls.h"

static const char *tag = "MAIN";


void app_main(void)
{
    tcp_tls_init();
    
    sys_initializer_init();
    
    wifi_ap_init();
}

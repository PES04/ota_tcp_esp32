#include <stdio.h>

#include "sys_initializer.h"
#include "wifi_ap.h"


static const char *tag = "MAIN";


void app_main(void)
{
    sys_initializer_run();
    
    wifi_ap_init();
}

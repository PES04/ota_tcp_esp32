#ifndef _WIFI_AP_H
#define _WIFI_AP_H

#define WIFI_AP_SSID_MAX_LEN        (32U)
#define WIFI_AP_PASSWORD_MAX_LEN    (64U)

void wifi_ap_init(void);

void wifi_ap_set_ssid(char *ssid, const uint8_t len);

void wifi_ap_set_password(char *password, const uint8_t len);

#endif

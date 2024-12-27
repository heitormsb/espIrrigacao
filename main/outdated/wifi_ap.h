#ifndef WIFI_AP_H
#define WIFI_AP_H

#ifdef __cplusplus
extern "C" {
#endif

void init_wifi_ap(void);
void wifi_init_softap(void);
int wifi_init_sta(char *ssid, char *password);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_AP_H */

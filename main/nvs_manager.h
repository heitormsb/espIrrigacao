#ifndef NVS_HELPER_H
#define NVS_HELPER_H

void nvs_init();
esp_err_t nvs_write_int(const char *key, int value);
esp_err_t nvs_read_int(const char *key, int *value);
// void save_wifi_credentials_to_nvs(const char *ssid, const char *password);
int read_wifi_ssid(char *ssid);
int read_wifi_password(char *password);
void write_string_to_nvs(const char *nvs_namespace, const char *key, const char *value);
void nvs_reset();

#endif // NVS_HELPER_H

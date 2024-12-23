#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_event.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"

// Credenciais do Wi-Fi
#define EXAMPLE_ESP_WIFI_SSID "your_ssid"
#define EXAMPLE_ESP_WIFI_PASS "pass1234"
#define EXAMPLE_MAX_STA_CONN  4

#define ESP_WIFI_SSID "VIVOFIBRA-029A"
#define ESP_WIFI_PASS "heitor12"

// Bits para o EventGroup
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// Variáveis compartilhadas
extern EventGroupHandle_t s_wifi_event_group;
extern int ap_sta;

// Declarações de funções
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
int from_sta_to_ap(void);
int from_ap_to_sta(char *ssid, char *password);

#endif // WIFI_MANAGER_H

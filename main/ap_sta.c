#include "esp_wifi.h"
#include "esp_log.h"
#include <string.h>
#include "lwip/ip4_addr.h" // Add this include for IP4_ADDR
#include "esp_netif.h" // Add this include for esp_netif functions
#include "freertos/FreeRTOS.h" // Add this include for FreeRTOS functions
#include "freertos/event_groups.h" // Add this include for event groups

#define EXAMPLE_ESP_WIFI_SSID "your_ssid"
#define EXAMPLE_ESP_WIFI_PASS "pass1234"
#define EXAMPLE_MAX_STA_CONN 4

# define ESP_WIFI_SSID "VIVOFIBRA-029A"
# define ESP_WIFI_PASS "heitor12"

#define WIFI_CONNECTED_BIT BIT0 // Add this definition
#define WIFI_FAIL_BIT BIT1 // Add this definition

int ap_sta = 0;
static const char *TAG = "wifi ap_sta";
static esp_netif_t *wifiAP; // Declare wifiAP
static EventGroupHandle_t s_wifi_event_group; // Declare s_wifi_event_group

void from_sta_to_ap(void){
    printf("## Entering from_sta_to_ap.\n");
    esp_netif_ip_info_t ipInfo;
    if(ap_sta == 1){
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    }
    IP4_ADDR(&ipInfo.ip, 192,168,2,1);
    IP4_ADDR(&ipInfo.gw, 192,168,2,1);
    IP4_ADDR(&ipInfo.netmask, 255,255,255,0);
    esp_netif_dhcps_stop(wifiAP);
    esp_netif_set_ip_info(wifiAP, &ipInfo);
    esp_netif_dhcps_start(wifiAP);
    ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&ipInfo.ip));
    ESP_LOGI(TAG, "GW: " IPSTR, IP2STR(&ipInfo.gw));
    ESP_LOGI(TAG, "Mask: " IPSTR, IP2STR(&ipInfo.netmask));
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    ap_sta = 0;
}

void from_ap_to_sta(void){
    printf("## Entering from_ap_to_sta.\n");
    if(ap_sta == 0){
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    }
    s_wifi_event_group = xEventGroupCreate();
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE,
            pdFALSE, portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", ESP_WIFI_SSID, ESP_WIFI_PASS);
    }
    else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", ESP_WIFI_SSID, ESP_WIFI_PASS);
    }
    else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    vEventGroupDelete(s_wifi_event_group);
    ap_sta = 1;
}
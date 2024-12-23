#include "wifi_manager.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include <string.h>
#include "mdns_wifi.h"

static const char* TAG = "WiFiManager";

EventGroupHandle_t s_wifi_event_group = NULL;
int ap_sta = 0;
// static esp_netif_t *wifiAP = NULL;


//***************************************************************
//                      WIFI EVENT HANDLER
//***************************************************************
void wifi_event_handler(void* arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void* event_data)
{
    // --- Eventos de AP ---
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        const wifi_event_ap_staconnected_t* event =
                (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        const wifi_event_ap_stadisconnected_t* event =
                (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }

    // --- Eventos de STA ---
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // Quando a Station é iniciada, tenta conectar:
        esp_wifi_connect();
        ESP_LOGI(TAG, "Station start event, trying to connect...");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // Quando a Station cai ou não consegue conectar
        // Aqui podemos tentar reconectar ou setar o bit de falha
        ESP_LOGI(TAG, "Station disconnected. Setting WIFI_FAIL_BIT...");
        if (s_wifi_event_group) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // Quando a Station pega IP
        const ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Station got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        if (s_wifi_event_group) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
}


//***************************************************************
//                  FUNÇÃO: from_sta_to_ap
//***************************************************************
int from_sta_to_ap(void)
{
    // Vamos trocar do modo STA para AP
    ESP_LOGI(TAG, "## Entering from_sta_to_ap. ap_sta = %d", ap_sta);

    // Se estávamos em STA, paramos a Station
    if (ap_sta == 1) {
        stop_mdns();
        // Desconecta, para e bota WIFI_MODE_NULL antes de subir AP
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    }

    // Configuração de IP estático no AP
    // esp_netif_ip_info_t ipInfo;
    // IP4_ADDR(&ipInfo.ip, 192,168,2,1);
    // IP4_ADDR(&ipInfo.gw, 192,168,2,1);
    // IP4_ADDR(&ipInfo.netmask, 255,255,255,0);

    // Ajusta a rede do AP
    // esp_netif_dhcps_stop(wifiAP);
    // esp_netif_set_ip_info(wifiAP, &ipInfo);
    // esp_netif_dhcps_start(wifiAP);

    // ESP_LOGI(TAG, "IP: " IPSTR,    IP2STR(&ipInfo.ip));
    // ESP_LOGI(TAG, "GW: " IPSTR,    IP2STR(&ipInfo.gw));
    // ESP_LOGI(TAG, "Mask: " IPSTR,  IP2STR(&ipInfo.netmask));

    // Configura as credenciais do AP
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

    // Sobe o Wi-Fi em modo AP
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);

    initialise_mdns();

    // Indica que agora estamos em modo AP
    ap_sta = 0;

    return 1;
}


//***************************************************************
//                  FUNÇÃO: from_ap_to_sta
//***************************************************************
int from_ap_to_sta(char *ssid, char *password)
{
    // Vamos trocar do modo AP para STA
    ESP_LOGI(TAG, "## Entering from_ap_to_sta. ap_sta = %d", ap_sta);

    // Se estava em AP, parar o Wi-Fi e modo NULL
    if (ap_sta == 0) {
        stop_mdns();
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    }

    // Cria o EventGroup para esperar conexão ou falha
    s_wifi_event_group = xEventGroupCreate();

    // Configura as credenciais de STA
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = {ssid},
            .password = {password},
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    strncpy((char*)wifi_config.sta.ssid,(char*)ssid, 32);
    strncpy((char*)wifi_config.sta.password,(char*)password, 32);

    ESP_LOGI(TAG, "Iniciando Wi-Fi em modo STA...");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Inicia a Station
    ESP_ERROR_CHECK(esp_wifi_start());

    // Aguarda até conectar (WIFI_CONNECTED_BIT) ou falhar (WIFI_FAIL_BIT)
    EventBits_t bits = xEventGroupWaitBits(
            s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
        initialise_mdns();


    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    // Deleta o EventGroup após receber o resultado
    vEventGroupDelete(s_wifi_event_group);
    s_wifi_event_group = NULL;

    // Indica que agora estamos em modo STA
    ap_sta = 1;

    return (bits & WIFI_CONNECTED_BIT) ? 1 : 0;
}
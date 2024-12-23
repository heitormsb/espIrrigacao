#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "protocol_examples_common.h"
#include "esp_sntp.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include <esp_mac.h>

// #include "wifi_ap.h"
// #include "wifi_sta.h"
// #include "test_wifi.h"
// #include "ap_sta.h"
#include "wifi_manager.h"
#include "tcp_server.h"
// #include "mdns_wifi.h"



#define PORT                        CONFIG_EXAMPLE_PORT
#define KEEPALIVE_IDLE              CONFIG_EXAMPLE_KEEPALIVE_IDLE
#define KEEPALIVE_INTERVAL          CONFIG_EXAMPLE_KEEPALIVE_INTERVAL
#define KEEPALIVE_COUNT             CONFIG_EXAMPLE_KEEPALIVE_COUNT

static const char *TAG = "example";
extern TaskHandle_t TcpHandle;

// ssid and password from tcp
char ssid_g[32] = "";
char password_g[32] = "";

extern int ap_sta;

static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;


//***************************************************************
//                       APP_MAIN
//***************************************************************
void app_main(void)
{
    // 1. Inicialização do NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Inicialização da pilha de rede
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 3. Inicialização do Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 4. Registro de eventos Wi-Fi
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        &instance_any_id
    ));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        &instance_got_ip
    ));

    // 5. Cria interfaces default (AP e STA)
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    // Exemplo de fluxo: AP -> STA -> AP -> STA
    ESP_LOGI(TAG, "## Entering from_sta_to_ap (primeira vez).");
    from_sta_to_ap();
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, &TcpHandle);


    ESP_LOGI(TAG, "## Fim do exemplo.");
}

// void app_main(void){
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//       ESP_ERROR_CHECK(nvs_flash_erase());
//       ret = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK(ret);

//     // char ssid_g1[32] = "VIVOFIBRA-029A";
//     // char password_g1[32] = "heitor12";
//     // wifi_init_station(ssid_g, password_g);

//     int connection_status = 0;

//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     initialise_mdns();
    
//     // xTaskCreate(wifi_ap_task, "wifi_ap", 8192, (void*)AF_INET, 5, &TcpHandle);
//     // init_ap("null", "null", 0);
//     xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, &TcpHandle);

//     // esp_wifi_stop();

//     while (1) {
//         if (strlen(password_g) > 5) {
//             ESP_LOGI(TAG, "SSID: %s, Password: %s", ssid_g, password_g);
//             ESP_LOGI(TAG, "Connect to WiFi station");
//             connection_status = init_ap(ssid_g, password_g, 1);
//             printf("Connection status: %d\n", connection_status);
//             break;
//         }
//         vTaskDelay(5000 / portTICK_PERIOD_MS);
//     }

//     esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
//     esp_netif_sntp_init(&config);
//     if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
//         printf("Failed to update system time within 10s timeout");
//     }
//     setenv("TZ", "GMT+3", 1);
//     tzset();

//     xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, &TcpHandle);



// // #ifdef CONFIG_EXAMPLE_IPV4
// //     xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, &TcpHandle);
// // #endif



// // #ifdef CONFIG_EXAMPLE_IPV6
// //     xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET6, 5, &TcpHandle);
// // #endif
// }
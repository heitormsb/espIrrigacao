#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include <string.h>

#include "wifi_manager.h"
#include "tcp_wifi_manager.h"
#include "nvs_manager.h"

#include "sntp_server.h"

static const char *TAG = "main";

extern int ap_sta;

static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;

extern TaskHandle_t WifiTcpHandle;
extern bool need_switch_to_sta;
extern bool need_switch_to_ap;

//***************************************************************
//                       APP_MAIN
//***************************************************************
void app_main(void){

    // 1. Inicialização do NVS
    nvs_init();
    // nvs_reset();

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


    // 6. Inicialização gerenciador Wi-Fi com TCP
    xTaskCreate(tcp_wifi_manager, "tcp_wifi_manager", 4096, NULL, 5, &WifiTcpHandle);

    // read wifi credentials from nvs
    char ssid_nvs[32];
    char password_nvs[32];
    bool ssid_read = read_wifi_ssid(ssid_nvs);
    bool password_read = read_wifi_password(password_nvs);

    if (!ssid_read && !password_read){
        ESP_LOGI(TAG, "Falha ao ler credenciais wifi do NVS");  
        need_switch_to_ap = true; 
    }
    else{
        if (strlen(ssid_nvs) > 0 && strlen(password_nvs) > 0){
            ESP_LOGI(TAG, "Credenciais lidas com sucesso. SSID: %s, Password: %s", ssid_nvs, password_nvs);
            need_switch_to_sta = true;
        } else {
            ESP_LOGI(TAG, "Credenciais Wi-Fi não encontradas no NVS");
            need_switch_to_ap = true;
        }
    }

    // 7. Inicialização do SNTP
    // setenv("TZ", "BRT3", 1); // Timezone string
    // tzset();
    // obtain_time();

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    printf("Current time: %s", asctime(&timeinfo));




    //***************************************************************
    //                  CPU/MEMORY MONITORING
    // Como ativar:
    //  Ativar no menuconfig "FreeRTOS" -> "Kernel"
    //  - FREERTOS_USE_TRACE_FACILITY
    //  - FREERTOS_GENERATE_RUN_TIME_STATS
    //  - FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
    //***************************************************************

    // while (1) {
    //     static char buffer[1024];
    //     vTaskGetRunTimeStats(buffer);
    //     ESP_LOGI(TAG, "\nTask CPU usage:\n%s", buffer);

    //     size_t free_heap = esp_get_free_heap_size();
    //     ESP_LOGI(TAG, "Free heap size: %u bytes", free_heap);

    //     size_t min_free_heap = esp_get_minimum_free_heap_size();
    //     ESP_LOGI(TAG, "Minimum free heap size: %u bytes", min_free_heap);
        
    //     vTaskDelay(pdMS_TO_TICKS(5000));
    // }

    ESP_LOGI(TAG, "## Fim do exemplo.");
}
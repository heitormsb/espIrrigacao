#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wifi_manager.h"
#include "tcp_server.h"
#include "lwip/sockets.h"
#include "nvs_manager.h"
#include "sntp_server.h"

static const char *TAG = "tcp_wifi_manager";

extern TaskHandle_t TcpHandle;

TaskHandle_t WifiTcpHandle = NULL;
bool need_switch_to_sta = false;
bool need_switch_to_ap = false;
bool tcp_server_running = false;
bool stop_server = false;


void start_tcp_server(void) {
    ESP_LOGI(TAG, "Start TCP server...");

    xTaskCreate(tcp_server_task, 
                "tcp_server", 
                4096, 
                (void*)AF_INET, 
                5, 
                &TcpHandle);
    tcp_server_running = true;
}

void stop_tcp_server(void) {
    ESP_LOGI(TAG, "Request to stop server");
    // Sinaliza para o servidor sair
    stop_server = true;

    vTaskDelay(pdMS_TO_TICKS(1000));

    // Ajusta flags
    tcp_server_running = false;
    TcpHandle = NULL;
    stop_server = false;
}


void tcp_wifi_manager(void *pvParameters){
    char ssid_nvs[32];
    char pass_nvs[32];
    bool wifi_connected = false;

    while(1){
        if (need_switch_to_sta){
            ESP_LOGI(TAG, "Switching to STA");
            need_switch_to_sta = false;

            if (tcp_server_running)
                stop_tcp_server();

            read_wifi_ssid(ssid_nvs);
            read_wifi_password(pass_nvs);
            wifi_connected = from_ap_to_sta(ssid_nvs, pass_nvs);

            vTaskDelay(1000 / portTICK_PERIOD_MS);

            ESP_LOGI(TAG, "ap_sta = %d", wifi_connected);

            if (!wifi_connected){
                ESP_LOGI(TAG, "Failed to connect to Wi-Fi. Switching to AP");
                need_switch_to_ap = true;
            }

            start_tcp_server(); 

            // toda vez que mudar para STA, sincroniza o tempo
            setenv("TZ", "BRT3", 1);
            tzset();
            obtain_time();
        }
        if (need_switch_to_ap){
            ESP_LOGI(TAG, "Switching to AP");
            need_switch_to_ap = false;

            if (tcp_server_running)
                stop_tcp_server();

            from_sta_to_ap();
            start_tcp_server();
        }


        // UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        // ESP_LOGI(TAG, "Watermark (bytes left): %u", watermark);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
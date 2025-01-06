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
extern TaskHandle_t TimeSyncTaskHandle;

extern int ap_sta;
const uint16_t init_time_retry = 60; // Tempo inicial para verificar conexão no modo station (60 segundos)
const uint16_t max_time_retry = 600; // Tempo máximo de retry (10 minutos)

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
    int retry_interval = init_time_retry;
    int ap_retry_timer = 0; // Temporizador para gerenciar o intervalo de verificação no modo AP


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
            } else {
                retry_interval = init_time_retry; // Reseta o intervalo em caso de sucesso
            }

            start_tcp_server(); 

            xTaskCreate(&periodic_time_sync_task, "periodic_time_sync_task", 4096, NULL, 5, &TimeSyncTaskHandle);
        }
        if (need_switch_to_ap){
            ESP_LOGI(TAG, "Switching to AP");
            need_switch_to_ap = false;
            wifi_connected = false;

            // Finaliza a tarefa de sincronização, se estiver rodando
            if (TimeSyncTaskHandle != NULL) {
                ESP_LOGI(TAG, "Finalizando tarefa de sincronização de tempo");
                vTaskDelete(TimeSyncTaskHandle);
                TimeSyncTaskHandle = NULL;
            }

            if (tcp_server_running)
                stop_tcp_server();

            from_sta_to_ap();
            start_tcp_server();
        }

        if (ap_sta == 0) { // Se estiver no modo AP, verifica periodicamente a conexão
            ap_retry_timer++;

            if (ap_retry_timer >= retry_interval) {
                ap_retry_timer = 0; // Reinicia o temporizador
                ESP_LOGI(TAG, "AP Mode: Checking periodically for STA connection (retry_interval = %d), (ap_retry_timer = %d)", retry_interval, ap_retry_timer);

                need_switch_to_sta = true;
                retry_interval = (retry_interval < max_time_retry) ? (retry_interval + 60) : max_time_retry;
                ESP_LOGI(TAG, "Next retry in %d seconds", retry_interval);
            }
        }

        // UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        // ESP_LOGI(TAG, "Watermark (bytes left): %u", watermark);
        vTaskDelay(1000 / portTICK_PERIOD_MS);  
    }
}
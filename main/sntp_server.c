// #include <stdio.h>
// #include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"
#include "esp_log.h"

static const char *TAG = "sntp_server";

TaskHandle_t TimeSyncTaskHandle = NULL;

#define MAX_RETRIES 10
#define NUM_SERVERS 4

// Lista de servidores NTP
const char *ntp_servers[NUM_SERVERS] = {
    "pool.ntp.org",
    "time.google.com",
    "a.st1.ntp.br",
    "b.st1.ntp.br"
};

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Tempo foi sincronizado");
}

void initialize_sntp(const char *server)
{
    ESP_LOGI(TAG, "Inicializando SNTP com o servidor: %s", server);

    // Pare o SNTP se já estiver rodando
    if (esp_sntp_enabled()) {
        ESP_LOGI(TAG, "Parando cliente SNTP existente...");
        esp_sntp_stop();
    }

    // Configura o cliente SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, server);
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);

    // Inicializa o SNTP
    esp_sntp_init();
}

bool sync_time_with_server(const char *server)
{
    initialize_sntp(server);

    // Aguarde até o tempo ser sincronizado ou atingir o limite de tentativas
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    while (timeinfo.tm_year < (2023 - 1900) && retry < MAX_RETRIES) {
        ESP_LOGI(TAG, "Aguardando sincronização do tempo... (%d/%d)", retry + 1, MAX_RETRIES);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
        retry++;
    }

    if (timeinfo.tm_year >= (2023 - 1900)) {
        ESP_LOGI(TAG, "Tempo sincronizado com sucesso com o servidor: %s", server);
        return true;
    }

    ESP_LOGE(TAG, "Falha ao sincronizar tempo com o servidor: %s", server);
    esp_sntp_stop();
    return false;
}

void obtain_time()
{
    bool synced = false;

    // Tente sincronizar com cada servidor da lista
    for (int i = 0; i < NUM_SERVERS; i++) {
        synced = sync_time_with_server(ntp_servers[i]);
        if (synced) {
            break; // Se sincronizar, pare de tentar outros servidores
        }
    }

    if (!synced) {
        printf("Failed to synchronize time with all servers\n");
    }
}

void periodic_time_sync_task(void *pvParameter){
    setenv("TZ", "BRT3", 1);
    tzset();

    while (1) {
        obtain_time();

        // UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        // ESP_LOGI(TAG, "Watermark (bytes left): %u", watermark);

        // rode de 24 em 24 horas
        // vTaskDelay(24 * 60 * 60 * 1000 / portTICK_PERIOD_MS);
        vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);
    }
}
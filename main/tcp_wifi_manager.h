#ifndef WIFI_TCP_MANAGER_H
#define WIFI_TCP_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Extern para o handle da task TCP
extern TaskHandle_t TcpHandle;

// Flags para controle do Wi-Fi e servidor TCP
extern TaskHandle_t WifiTcpHandle;
extern bool need_switch_to_sta;
extern bool need_switch_to_ap;
extern bool tcp_server_running;
extern bool stop_server;

// Funções para gerenciamento do servidor TCP
void start_tcp_server(void);
void stop_tcp_server(void);

// Task principal de gerenciamento Wi-Fi e TCP
void tcp_wifi_manager(void *pvParameters);

#endif // WIFI_TCP_MANAGER_H

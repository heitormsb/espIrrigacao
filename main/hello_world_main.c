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

// #include "wifi_ap.h"
// #include "wifi_sta.h"
#include "test_wifi.h"
#include "mdns_wifi.h"


#define PORT                        CONFIG_EXAMPLE_PORT
#define KEEPALIVE_IDLE              CONFIG_EXAMPLE_KEEPALIVE_IDLE
#define KEEPALIVE_INTERVAL          CONFIG_EXAMPLE_KEEPALIVE_INTERVAL
#define KEEPALIVE_COUNT             CONFIG_EXAMPLE_KEEPALIVE_COUNT

static const char *TAG = "example";
TaskHandle_t TcpHandle = NULL;

// ssid and password from tcp
char ssid_g[32] = "";
char password_g[32] = "";

static void do_retransmit(const int sock, const int listen_sock){
    int len;
    char rx_buffer[128];

    memset(rx_buffer, 0, sizeof(rx_buffer));

    do {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGW(TAG, "Connection closed");
        } else {
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

            if (strcmp(rx_buffer, "0\n") == 0) {
                ESP_LOGI(TAG, "Received 0, closing connection");
                close(listen_sock);
                // close(sock);
                vTaskDelete(TcpHandle);
            } 

            else if (strncmp(rx_buffer, "GW", 2) == 0) {

                for (int i = 0; i < len; i++) {
                    printf("%02X ", (unsigned char)rx_buffer[i]);
                }   
                printf("\n");

                ESP_LOGI(TAG, "Received GW, get WiFi data");
                char *ptr = rx_buffer + 2;
                char *ssid = ptr;
                char *password = NULL;
                while (*ptr != ';') {
                    ptr++;
                }
                *ptr = '\0';
                password = ptr + 1;
                if (password[strlen(password) - 1] == '\n')
                    password[strlen(password) - 1] = '\0';

                strcpy(ssid_g, ssid);
                strcpy(password_g, password);
                
                // 02X for ssid and password
                for (int i = 0; i < strlen(ssid); i++) {
                    printf("%02X ", (unsigned char)ssid[i]);
                }
                printf("\n");
                for (int i = 0; i < strlen(password); i++) {
                    printf("%02X ", (unsigned char)password[i]);
                }
                printf("\n");

                ESP_LOGI(TAG, "SSID: %s, Password: %s", ssid, password);

                // close wifi ap
                ESP_LOGI(TAG, "Close WiFi AP");
                // esp_wifi_stop();
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                // close tcp server
                close(listen_sock);
                // close(sock);
                vTaskDelete(TcpHandle);
            }

            else if (strncmp(rx_buffer, "TIME", 2) == 0) {
                // get time
                time_t now;
                struct tm timeinfo;
                time(&now);
                localtime_r(&now, &timeinfo);
                char strftime_buf[64];
                strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
                ESP_LOGI(TAG, "The current date/time in Sao Paulo is: %s", strftime_buf);
                send(sock, strftime_buf, strlen(strftime_buf), 0);
                
            }


        }
    } while (len > 0);

}

static void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;

#ifdef CONFIG_EXAMPLE_IPV4
    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;
    }
#endif
#ifdef CONFIG_EXAMPLE_IPV6
    if (addr_family == AF_INET6) {
        struct sockaddr_in6 *dest_addr_ip6 = (struct sockaddr_in6 *)&dest_addr;
        bzero(&dest_addr_ip6->sin6_addr.un, sizeof(dest_addr_ip6->sin6_addr.un));
        dest_addr_ip6->sin6_family = AF_INET6;
        dest_addr_ip6->sin6_port = htons(PORT);
        ip_protocol = IPPROTO_IPV6;
    }
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
    // Note that by default IPV6 binds to both protocols, it is must be disabled
    // if both protocols used at the same time (used in CI)
    setsockopt(listen_sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
#endif

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {

        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        // Convert ip address to string
#ifdef CONFIG_EXAMPLE_IPV4
        if (source_addr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
#endif
#ifdef CONFIG_EXAMPLE_IPV6
        if (source_addr.ss_family == PF_INET6) {
            inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
        }
#endif
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        do_retransmit(sock, listen_sock);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}


// static void wifi_ap_task(void *pvParameters){
//     init_wifi_ap();
//     //init_mdns();
//     // tcp_server_task(pvParameters);
// }

void app_main(void){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // char ssid_g1[32] = "VIVOFIBRA-029A";
    // char password_g1[32] = "heitor12";
    // wifi_init_station(ssid_g, password_g);

    int connection_status = 0;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    initialise_mdns();
    
    // xTaskCreate(wifi_ap_task, "wifi_ap", 8192, (void*)AF_INET, 5, &TcpHandle);
    init_ap("null", "null", 0);
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, &TcpHandle);

    // esp_wifi_stop();

    while (1) {
        if (strlen(password_g) > 5) {
            ESP_LOGI(TAG, "SSID: %s, Password: %s", ssid_g, password_g);
            ESP_LOGI(TAG, "Connect to WiFi station");
            connection_status = init_ap(ssid_g, password_g, 1);
            printf("Connection status: %d\n", connection_status);
            break;
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);
    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
        printf("Failed to update system time within 10s timeout");
    }
    setenv("TZ", "GMT+3", 1);
    tzset();

    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, &TcpHandle);



// #ifdef CONFIG_EXAMPLE_IPV4
//     xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, &TcpHandle);
// #endif



// #ifdef CONFIG_EXAMPLE_IPV6
//     xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET6, 5, &TcpHandle);
// #endif
}
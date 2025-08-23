#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "connection_component.h"
#include "parsing_component.h"

// #define SERVER_IP "192.168.137.155"
#define SERVER_IP "192.168.2.42"
// #define SERVER_PORT 5000
#define SERVER_PORT 5001

#define WIFI_SSID "LOUISA COFFEE 1F_23085252"
#define WIFI_PASSWORD "23085252"
// "LAPTOP-1L16PLG5 3420" // your wifi ssid
// "66666666",         // your wifi password

static const char *TAG = "TCP COMPONENT";

void tcp_client_task(void *pvParameters)
{
    char rx_buffer[1024];
    struct sockaddr_in dest_addr;
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    while (sock < 0)
    {
        ESP_LOGE(TAG, "socket fail errno %d, retrying...", errno);
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        vTaskDelay(pdMS_TO_TICKS(500));
        // vTaskDelete(NULL);
        // return;
    }
    dest_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SERVER_PORT);
    ESP_LOGI(TAG, "Connecting to %s:%d", SERVER_IP, SERVER_PORT);
    while (connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0)
    {
        ESP_LOGE(TAG, "connect fail errno %d, retrying...", errno);
        vTaskDelay(pdMS_TO_TICKS(500));

        // close(sock);
        // vTaskDelete(NULL);
        // return;
    }

    // initial sync request
    send_sync_with_t1(sock);

    char accum[1024];
    size_t acc_len = 0;
    while (1)
    {
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0)
        {
            ESP_LOGE(TAG, "recv errno %d", errno);
            break;
        }
        if (len == 0)
        {
            ESP_LOGW(TAG, "server closed");
            break;
        }
        rx_buffer[len] = '\0';
        for (int i = 0; i < len; i++)
        {
            char ch = rx_buffer[i];
            if (ch == '\n')
            {
                accum[acc_len] = '\0';
                if (acc_len > 0)
                    process_line(sock, accum);
                acc_len = 0;
            }
            else if (acc_len < sizeof(accum) - 1)
            {
                accum[acc_len++] = ch;
            }
        }
    }
    close(sock);
    vTaskDelete(NULL);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Wi-Fi disconnected, retrying...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: %s", ip4addr_ntoa((ip4_addr_t *)&event->ip_info.ip));
        xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
    }
}
void connection_start(void)
{
    // initialization of wifi setting

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers for Wi-Fi and IP events
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,         // your wifi ssid
            .password = WIFI_PASSWORD, // your wifi password
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Wi-Fi started");
}

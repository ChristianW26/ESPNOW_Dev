/* ESPNOW Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
   This example shows how to use ESPNOW.
   Prepare two device, one for sending ESPNOW data and another for receiving
   ESPNOW data.
*/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "espnow_example.h"

static const char *TAG = "espnow_example";

/* ----------------------------------- Private Global Variables ----------------------------------- */
static uint8_t s_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint8_t s_rx_mac[ESP_NOW_ETH_ALEN];
static uint8_t s_tx_mac[ESP_NOW_ETH_ALEN];

/* ----------------------------------- Private Functions ----------------------------------- */
static void mac_str_to_bytes(char *mac_str, uint8_t *mac_bytes) {
    sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
        mac_bytes, mac_bytes+1, mac_bytes+2, mac_bytes+3, mac_bytes+4, mac_bytes+5); 
}

/* WiFi should start before using ESPNOW */
static void example_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());
    ESP_ERROR_CHECK( esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
}

/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void example_espnow_send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
    if (tx_info == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    char data[tx_info->data_len];
    memcpy(data, tx_info->data, tx_info->data_len);  

    ESP_LOGI(TAG, "Sending message from "MACSTR" to "MACSTR, MAC2STR(tx_info->src_addr), MAC2STR(tx_info->des_addr));
    ESP_LOGI(TAG, "Message: %s", data);
}

static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    uint8_t * mac_addr = recv_info->src_addr;

    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    ESP_LOGI(TAG, "Receiving message from "MACSTR" to "MACSTR, MAC2STR(recv_info->src_addr), MAC2STR(recv_info->des_addr));
    char msg[len]; 
    memcpy(msg, data, len); 
    ESP_LOGI(TAG, "Message: %s", msg);
}

static void example_espnow_task(void *pvParameter)
{
    for (;;) {
        #ifdef CONFIG_TX_DEVICE
        char data_buf[CONFIG_MSG_LENGTH];
        strncpy(data_buf, CONFIG_MSG_DATA, CONFIG_MSG_LENGTH);
        esp_err_t status = esp_now_send(s_broadcast_mac, (uint8_t *) data_buf, CONFIG_MSG_LENGTH);
        if (status != ESP_OK) {
            ESP_LOGE(TAG, "Send error: %s", esp_err_to_name(status));
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        #endif

        #ifdef CONFIG_RX_DEVICE
        ESP_LOGI(TAG, "Waiting");
        vTaskDelay(1000 / portTICK_PERIOD_MS); 
        #endif
    }
}

static esp_err_t example_espnow_init(void)
{
    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );
#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
    ESP_ERROR_CHECK( esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW) );
    ESP_ERROR_CHECK( esp_wifi_connectionless_module_set_wake_interval(CONFIG_ESPNOW_WAKE_INTERVAL) );
#endif
    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;

    /* Set peer MAC addr */
    char tx_mac_str[] = CONFIG_TX_MAC_ADDR; 
    char rx_mac_str[] = CONFIG_RX_MAC_ADDR; 
    mac_str_to_bytes(tx_mac_str, s_tx_mac); 
    mac_str_to_bytes(rx_mac_str, s_rx_mac); 
    #ifdef CONFIG_TX_DEVICE
        memcpy(peer->peer_addr, s_rx_mac, ESP_NOW_ETH_ALEN);
    #elif defined(CONFIG_RX_DEVICE)
        memcpy(peer->peer_addr, s_tx_mac, ESP_NOW_ETH_ALEN);
    #else 
        memcpy(peer->peer_addr, s_broadcast_mac, ESP_NOW_ETH_ALEN);
    #endif 

    ESP_ERROR_CHECK( esp_now_add_peer(peer) );
    free(peer);

    return ESP_OK;
}

/* ----------------------------------- Entry Point ----------------------------------- */
void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    example_wifi_init();
    example_espnow_init();
    
    xTaskCreate(example_espnow_task, "example_espnow_task", 2048, NULL, 4, NULL);

    for (;;) {
        vTaskDelay(2); 
    }
}

/* ESPNOW Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef ESPNOW_EXAMPLE_H
#define ESPNOW_EXAMPLE_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_now.h"

/* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */
#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

#define ESPNOW_QUEUE_SIZE           6

#define IS_BROADCAST_ADDR(addr) (memcmp(addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN) == 0)

// #define RX_MAC_ADDR  {0x78, 0x42, 0x1c, 0x94, 0xb4, 0xe0} // 78:42:1c:94:b4:e0
#define TX_MAC_ADDR  {0x24, 0xec, 0x4a, 0x52, 0xb3, 0x00} // 24:ec:4a:52:b3:00
// uint8_t dest_mac[ESP_NOW_ETH_ALEN] = RX_MAC_ADDR;

/*
menuconfig:  -> already in ESP_NOW because of the Kconfig.projbuild file
wifi mode: station
primary master key: pmk123456789012
local master key: lmk1234567890123
channel: 8
send count: 100 (any big number?)
send delay: 1000 (in ms)
send len: 10 (needs to be <= 250)
enable long range: no
enable power save: no
*/

/* Broadcast address used by the example */
extern const uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN];

#define IS_BROADCAST_ADDR(addr) (memcmp(addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN) == 0)

/*
 * STATIC PEER MAC:
 * Replace the bytes below with the peer's MAC you want to communicate with.
 */
#define STATIC_PEER_MAC TX_MAC_ADDR  // if set to tx, i am rx


/* Event IDs */
typedef enum {
    EXAMPLE_ESPNOW_SEND_CB,
    EXAMPLE_ESPNOW_RECV_CB,
} example_espnow_event_id_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    esp_now_send_status_t status;
} example_espnow_event_send_cb_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t *data;
    int data_len;
} example_espnow_event_recv_cb_t;

typedef union {
    example_espnow_event_send_cb_t send_cb;
    example_espnow_event_recv_cb_t recv_cb;
} example_espnow_event_info_t;

/* When ESPNOW sending or receiving callback function is called, post event to ESPNOW task. */
typedef struct {
    example_espnow_event_id_t id;
    example_espnow_event_info_t info;
} example_espnow_event_t;

enum {
    EXAMPLE_ESPNOW_DATA_BROADCAST,
    EXAMPLE_ESPNOW_DATA_UNICAST,
    EXAMPLE_ESPNOW_DATA_MAX,
};

/* User defined field of ESPNOW data in this example. */
typedef struct {
    uint8_t type;                         // Broadcast or unicast ESPNOW data.
    uint8_t state;                        // Indicate whether it has received broadcast ESPNOW data or not.
    uint16_t seq_num;                     // Sequence number of ESPNOW data.
    uint16_t crc;                         // CRC16 value of ESPNOW data.
    uint32_t magic;                       // Magic number which is used to determine which device sends unicast ESPNOW data.
    uint8_t payload[0];                   // Real payload of ESPNOW data.
} __attribute__((packed)) example_espnow_data_t;

/* Parameters of sending ESPNOW data. */
typedef struct {
    bool unicast;                         // Send unicast ESPNOW data.
    bool broadcast;                       // Send broadcast ESPNOW data.
    uint8_t state;                        // Indicate whether it has received broadcast ESPNOW data or not.
    uint32_t magic;                       // Magic number which is used to determine which device sends unicast ESPNOW data.
    uint16_t count;                       // Total count of unicast ESPNOW data to be sent.
    uint16_t delay;                       // Delay between sending two ESPNOW data, unit: ms.
    int len;                              // Length of ESPNOW data to be sent, unit: byte.
    uint8_t *buffer;                      // Buffer pointing to ESPNOW data.
    uint8_t dest_mac[ESP_NOW_ETH_ALEN];   // MAC address of destination device.
} example_espnow_send_param_t;

/* ------------------------------------------- Public Function Definitions ------------------------------------------- */
void test_wifi(void); 

#endif /* ESPNOW_EXAMPLE_H */

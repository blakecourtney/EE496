//DRONE

/* Mesh Internal Communication Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <inttypes.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "mesh_light.h"
#include "nvs_flash.h"
#include "config.h" //packet specs
#include "driver/uart.h"
#include "mavlink/common/mavlink.h"

/*******************************************************
 *                Macros
 *******************************************************/

/*******************************************************
 *                Constants
 *******************************************************/
#define RX_SIZE          (1500)
#define TX_SIZE          (1460)

/*******************************************************
 *                Variable Definitions
 *******************************************************/
static const char *MESH_TAG = "mesh_main";
static const uint8_t s_mesh_id[] = MESH_ID;
static uint8_t rx_buf[RX_SIZE] = { 0, };
static bool is_running = true;
static bool is_mesh_connected = false;
static mesh_addr_t mesh_parent_addr;
static int mesh_layer = -1;
static esp_netif_t *netif_sta = NULL;
static mesh_addr_t root_addr;
static bool root_addr_known = false;

/*******************************************************
 *                Function Declarations
 *******************************************************/

/*******************************************************
 *                Function Definitions
 *******************************************************/
void esp_mesh_p2p_tx_main(void *arg)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    uint8_t drone_id = mac[5];
    is_running = true;

    while (is_running) {
        if (!esp_mesh_is_root()) {
            if (root_addr_known) {
                packet_t pkt = {
                    .start    = PKT_START,
                    .drone_id = drone_id,
                    .end      = PKT_END,
                };
                memset(pkt.payload, 0, sizeof(pkt.payload));

                if (telemetry_valid) {
                    // send telemetry from MAVLink if possible
                    pkt.type = PKT_TYPE_TELEMETRY;
                    memcpy(pkt.payload, &latest_telemetry, sizeof(telemetry_t));
                    ESP_LOGI(MESH_TAG, "[DRONE-TX] telemetry lat:%.5f lon:%.5f alt:%.1f",
                             latest_telemetry.lat, latest_telemetry.lon, latest_telemetry.alt);
                } else {
                    // no MAVLink data, send heartbeat
                    pkt.type = PKT_TYPE_HEARTBEAT;
                    ESP_LOGI(MESH_TAG, "[DRONE-TX] heartbeat drone_id:%d (no telemetry yet)", drone_id);
                }

                mesh_data_t data = {
                    .data  = (uint8_t *)&pkt,
                    .size  = sizeof(pkt),
                    .proto = MESH_PROTO_BIN,
                    .tos   = MESH_TOS_P2P,
                };
                esp_err_t err = esp_mesh_send(&root_addr, &data, MESH_DATA_P2P, NULL, 0);
                if (err) {
                    ESP_LOGE(MESH_TAG, "[DRONE-TX] send failed err:0x%x", err);
                }
            } else {
                ESP_LOGI(MESH_TAG, "[DRONE-TX] waiting for root address...");
            }
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void esp_mesh_p2p_rx_main(void *arg)
{
    int recv_count = 0;
    esp_err_t err;
    mesh_addr_t from;
    mesh_data_t data;
    int flag = 0;
    data.data = rx_buf;
    data.size = RX_SIZE;
    is_running = true;

    while (is_running) {
        data.size = RX_SIZE;
        err = esp_mesh_recv(&from, &data, portMAX_DELAY, &flag, NULL, 0);
        if (err != ESP_OK || !data.size) {
            ESP_LOGE(MESH_TAG, "err:0x%x, size:%d", err, data.size);
            continue;
        }
        recv_count++;
        if (data.size >= sizeof(packet_t)) {
            packet_t *pkt = (packet_t *)data.data;
            if (pkt->start == PKT_START && pkt->end == PKT_END) {
                ESP_LOGW(MESH_TAG, "[DRONE-RX] #%d type:%d drone_id:%d from "MACSTR,
                         recv_count, pkt->type, pkt->drone_id, MAC2STR(from.addr));
            }
        }
    }
    vTaskDelete(NULL);
}

esp_err_t esp_mesh_comm_p2p_start(void)
{
    static bool is_comm_p2p_started = false;
    if (!is_comm_p2p_started) {
        is_comm_p2p_started = true;
        xTaskCreate(esp_mesh_p2p_tx_main, "MPTX", 3072, NULL, 5, NULL);
        xTaskCreate(esp_mesh_p2p_rx_main, "MPRX", 3072, NULL, 5, NULL);
    }
    return ESP_OK;
}

void mesh_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    mesh_addr_t id = {0,};
    static uint16_t last_layer = 0;

    switch (event_id) {
    case MESH_EVENT_STARTED: {
        esp_mesh_get_id(&id);
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_MESH_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_STOPPED: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOPPED>");
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_CHILD_CONNECTED: {
        mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, "MACSTR"",
                 child_connected->aid,
                 MAC2STR(child_connected->mac));
    }
    break;
    case MESH_EVENT_CHILD_DISCONNECTED: {
        mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, "MACSTR"",
                 child_disconnected->aid,
                 MAC2STR(child_disconnected->mac));
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_ADD: {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d, layer:%d",
                 routing_table->rt_size_change,
                 routing_table->rt_size_new, mesh_layer);
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE: {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d, layer:%d",
                 routing_table->rt_size_change,
                 routing_table->rt_size_new, mesh_layer);
    }
    break;
    case MESH_EVENT_NO_PARENT_FOUND: {
        mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
                 no_parent->scan_times);
    }
    /* TODO handler for the failure */
    break;
    case MESH_EVENT_PARENT_CONNECTED: {
        mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
        esp_mesh_get_id(&id);
        mesh_layer = connected->self_layer;
        memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:"MACSTR"%s, ID:"MACSTR", duty:%d",
                 last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
                 esp_mesh_is_root() ? "<ROOT>" :
                 (mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr), connected->duty);
        last_layer = mesh_layer;
        mesh_connected_indicator(mesh_layer);
        is_mesh_connected = true;
        if (esp_mesh_is_root()) {
            esp_netif_dhcpc_stop(netif_sta);
            esp_netif_dhcpc_start(netif_sta);
        }
        esp_mesh_comm_p2p_start();
    }
    break;
    case MESH_EVENT_PARENT_DISCONNECTED: {
        mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
                 disconnected->reason);
        is_mesh_connected = false;
        mesh_disconnected_indicator();
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_LAYER_CHANGE: {
        mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
        mesh_layer = layer_change->new_layer;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
                 last_layer, mesh_layer,
                 esp_mesh_is_root() ? "<ROOT>" :
                 (mesh_layer == 2) ? "<layer2>" : "");
        last_layer = mesh_layer;
        mesh_connected_indicator(mesh_layer);
    }
    break;
    case MESH_EVENT_ROOT_ADDRESS: {
        mesh_event_root_address_t *root_addr_event = (mesh_event_root_address_t *)event_data;
        memcpy(&root_addr.addr, root_addr_event->addr, 6);
        root_addr_known = true;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"",
                 MAC2STR(root_addr.addr));
    }
    break;
    case MESH_EVENT_VOTE_STARTED: {
        mesh_event_vote_started_t *vote_started = (mesh_event_vote_started_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"",
                 vote_started->attempts,
                 vote_started->reason,
                 MAC2STR(vote_started->rc_addr.addr));
    }
    break;
    case MESH_EVENT_VOTE_STOPPED: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STOPPED>");
        break;
    }
    case MESH_EVENT_ROOT_SWITCH_REQ: {
        mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"",
                 switch_req->reason,
                 MAC2STR( switch_req->rc_addr.addr));
    }
    break;
    case MESH_EVENT_ROOT_SWITCH_ACK: {
        /* new root */
        mesh_layer = esp_mesh_get_layer();
        esp_mesh_get_parent_bssid(&mesh_parent_addr);
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:"MACSTR"", mesh_layer, MAC2STR(mesh_parent_addr.addr));
    }
    break;
    case MESH_EVENT_TODS_STATE: {
        mesh_event_toDS_state_t *toDs_state = (mesh_event_toDS_state_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d", *toDs_state);
    }
    break;
    case MESH_EVENT_ROOT_FIXED: {
        mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_FIXED>%s",
                 root_fixed->is_fixed ? "fixed" : "not fixed");
    }
    break;
    case MESH_EVENT_ROOT_ASKED_YIELD: {
        mesh_event_root_conflict_t *root_conflict = (mesh_event_root_conflict_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d",
                 MAC2STR(root_conflict->addr),
                 root_conflict->rssi,
                 root_conflict->capacity);
    }
    break;
    case MESH_EVENT_CHANNEL_SWITCH: {
        mesh_event_channel_switch_t *channel_switch = (mesh_event_channel_switch_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", channel_switch->channel);
    }
    break;
    case MESH_EVENT_SCAN_DONE: {
        mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
                 scan_done->number);
    }
    break;
    case MESH_EVENT_NETWORK_STATE: {
        mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
                 network_state->is_rootless);
    }
    break;
    case MESH_EVENT_STOP_RECONNECTION: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOP_RECONNECTION>");
    }
    break;
    case MESH_EVENT_FIND_NETWORK: {
        mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"",
                 find_network->channel, MAC2STR(find_network->router_bssid));
    }
    break;
    case MESH_EVENT_ROUTER_SWITCH: {
        mesh_event_router_switch_t *router_switch = (mesh_event_router_switch_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, "MACSTR"",
                 router_switch->ssid, router_switch->channel, MAC2STR(router_switch->bssid));
    }
    break;
    case MESH_EVENT_PS_PARENT_DUTY: {
        mesh_event_ps_duty_t *ps_duty = (mesh_event_ps_duty_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_PS_PARENT_DUTY>duty:%d", ps_duty->duty);
    }
    break;
    case MESH_EVENT_PS_CHILD_DUTY: {
        mesh_event_ps_duty_t *ps_duty = (mesh_event_ps_duty_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_PS_CHILD_DUTY>cidx:%d, "MACSTR", duty:%d", ps_duty->child_connected.aid-1,
                MAC2STR(ps_duty->child_connected.mac), ps_duty->duty);
    }
    break;
    default:
        ESP_LOGI(MESH_TAG, "unknown id:%" PRId32 "", event_id);
        break;
    }
}

void ip_event_handler(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    ESP_LOGI(MESH_TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));

}

//MAVLINK Code for UART to Flight controller
#define MAV_UART     UART_NUM_0
#define MAV_TX_PIN   1
#define MAV_RX_PIN   3
#define MAV_BAUD     115200

// shared telemetry data, updated by mavlink task, read by mesh tx task
static telemetry_t latest_telemetry = {0};
static bool telemetry_valid = false;

void mavlink_init(void) {
    uart_config_t cfg = {
        .baud_rate = MAV_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(MAV_UART, &cfg);
    uart_set_pin(MAV_UART, MAV_TX_PIN, MAV_RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(MAV_UART, 2048, 0, 0, NULL, 0);
}

void mavlink_task(void *arg) {
    uint8_t byte;
    mavlink_message_t msg;
    mavlink_status_t status;

    while (1) {
        int len = uart_read_bytes(MAV_UART, &byte, 1, pdMS_TO_TICKS(10));
        if (len > 0) {
            if (mavlink_parse_char(MAVLINK_COMM_0, byte, &msg, &status)) {
                switch (msg.msgid) {

                    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
                        mavlink_global_position_int_t pos;
                        mavlink_msg_global_position_int_decode(&msg, &pos);
                        latest_telemetry.lat = pos.lat / 1e7f;
                        latest_telemetry.lon = pos.lon / 1e7f;
                        latest_telemetry.alt = pos.alt / 1000.0f;
                        telemetry_valid = true;
                        break;
                    }

                    case MAVLINK_MSG_ID_ATTITUDE: {
                        mavlink_attitude_t att;
                        mavlink_msg_attitude_decode(&msg, &att);
                        latest_telemetry.roll  = att.roll;
                        latest_telemetry.pitch = att.pitch;
                        latest_telemetry.yaw   = att.yaw;
                        break;
                    }

                    case MAVLINK_MSG_ID_SYS_STATUS: {
                        mavlink_sys_status_t sys;
                        mavlink_msg_sys_status_decode(&msg, &sys);
                        latest_telemetry.battery = sys.voltage_battery / 1000.0f;
                        break;
                    }

                    case MAVLINK_MSG_ID_GPS_RAW_INT: {
                        mavlink_gps_raw_int_t gps;
                        mavlink_msg_gps_raw_int_decode(&msg, &gps);
                        latest_telemetry.satellites = gps.satellites_visible;
                        break;
                    }

                    case MAVLINK_MSG_ID_HEARTBEAT: {
                        mavlink_heartbeat_t hb;
                        mavlink_msg_heartbeat_decode(&msg, &hb);
                        latest_telemetry.armed = (hb.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) ? 1 : 0;
                        break;
                    }
                }
            }
        }
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(mesh_light_init());
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(&netif_sta, NULL));

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mesh_set_topology(CONFIG_MESH_TOPOLOGY));
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(CONFIG_MESH_MAX_LAYER));

    ESP_ERROR_CHECK(esp_mesh_disable_ps()); 

    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    //pull mesh ID from config file 
    memcpy((uint8_t *) &cfg.mesh_id, s_mesh_id, 6);
    
    //"router config" : use dummy variables to avoid error
    cfg.channel = 1; 
    char dummy_ssid[] = "dummy_router";
    cfg.router.ssid_len = strlen(dummy_ssid);
    memcpy((uint8_t *) &cfg.router.ssid, dummy_ssid, cfg.router.ssid_len);
    memcpy((uint8_t *) &cfg.router.password, "dummy_password", strlen("dummy_password"));

    //drone specific config
    ESP_ERROR_CHECK(esp_mesh_set_self_organized(true, true)); // Allow auto-joining
    

    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(CONFIG_MESH_AP_AUTHMODE));
    cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
    memcpy((uint8_t *) &cfg.mesh_ap.password, CONFIG_MESH_AP_PASSWD, strlen(CONFIG_MESH_AP_PASSWD));
    
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg)); // Fixes the crash at line 391

    //start MAVLINK
    mavlink_init();
    xTaskCreate(mavlink_task, "MAVLink", 4096, NULL, 5, NULL);

    ESP_ERROR_CHECK(esp_mesh_start());

    ESP_LOGI(MESH_TAG, "DRONE NODE STARTED, SEARCHING FOR GROUND STATION...");
}
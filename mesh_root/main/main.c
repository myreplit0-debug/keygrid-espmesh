#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mesh_transport.h"
#include "app_proto.h"

static const char* TAG = "mesh_root";

static void send_ack(const mesh_addr_t* to, uint16_t seq) {
    app_ack_t ack = {.type = APP_TYPE_ACK, .seq = seq};
    mesh_tr_send(to, &ack, sizeof(ack));
}

static void on_mesh_rx(const mesh_addr_t* from, const uint8_t* data, uint16_t len) {
    if (len >= sizeof(app_report_t) && data[0] == APP_TYPE_REPORT) {
        const app_report_t* rpt = (const app_report_t*)data;
        ESP_LOGI(TAG, "REPORT room=%u seq=%u batch=%u rssi=%d "
                      "tag=%02X:%02X:%02X:%02X:%02X:%02X FROM " MACSTR,
                 rpt->room, rpt->seq, rpt->batch, rpt->rssi,
                 rpt->tag[0], rpt->tag[1], rpt->tag[2],
                 rpt->tag[3], rpt->tag[4], rpt->tag[5],
                 MAC2STR(from->addr));
        send_ack(from, rpt->seq);
    } else {
        ESP_LOGI(TAG, "RX %u bytes FROM " MACSTR, len, MAC2STR(from->addr));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Root starting…");

    mesh_cfg_simple_t cfg = {
        .mesh_id = {0x4B,0x47,0x01,0x02,0x03,0x04},
        .channel = 6,
    };
    strcpy(cfg.ap_pass, "keygrid123");

    mesh_tr_init(true, &cfg, on_mesh_rx);
}

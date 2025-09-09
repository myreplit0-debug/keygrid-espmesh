#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "mesh_transport.h"
#include "app_proto.h"

static const char* TAG = "mesh_root";

// RX callback: process reports from nodes and send ACK
static void on_mesh_rx(const mesh_addr_t* from, const uint8_t* data, uint16_t len) {
    if (len < sizeof(app_report_t)) return;

    const app_report_t* rpt = (const app_report_t*)data;
    if (rpt->type != APP_TYPE_REPORT) return;

    ESP_LOGI(TAG,
             "REPORT room=%u seq=%u batch=%u rssi=%d tag=%02X:%02X:%02X:%02X:%02X:%02X",
             (unsigned)rpt->room,
             (unsigned)rpt->seq,
             (unsigned)rpt->batch,
             (int)rpt->rssi,
             rpt->tag[0], rpt->tag[1], rpt->tag[2],
             rpt->tag[3], rpt->tag[4], rpt->tag[5]);

    // Build ACK
    app_ack_t ack = {
        .type = APP_TYPE_ACK,
        .seq  = rpt->seq,
    };
    mesh_tr_send(from, &ack, sizeof(ack));
}

void app_main(void) {
    ESP_LOGI(TAG, "Root starting…");

    // Mesh config
    mesh_cfg_simple_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.mesh_id[0] = 0x4B; // 'K'
    cfg.mesh_id[1] = 0x47; // 'G'
    cfg.mesh_id[2] = 0x01;
    cfg.mesh_id[3] = 0x02;
    cfg.mesh_id[4] = 0x03;
    cfg.mesh_id[5] = 0x04;
    cfg.channel    = 6;
    strcpy(cfg.ap_pass, "keygrid123"); // must match nodes

    mesh_tr_init(true /*force_root*/, &cfg, on_mesh_rx);
}

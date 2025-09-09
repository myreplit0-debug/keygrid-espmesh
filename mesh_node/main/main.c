#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"          // node uses esp_timer for ms timestamps

#include "mesh_transport.h"
#include "app_proto.h"

static const char* TAG = "mesh_node";

// App state
static uint16_t g_seq   = 1;
static uint32_t g_batch = 1000;
static uint8_t  g_room  = CONFIG_KG_ROOM_ID;

// In-flight tracking for ACK/Retry
static uint16_t inflight_seq = 0;
static uint32_t inflight_at  = 0;  // ms
static uint8_t  inflight_try = 0;

// RX callback from transport (called on RX task context)
static void on_mesh_rx(const mesh_addr_t* from, const uint8_t* data, uint16_t len) {
    uint16_t ack_seq = 0;
    if (app_parse_ack(data, len, &ack_seq) && ack_seq == inflight_seq) {
        ESP_LOGI(TAG, "ACK seq=%u from " MACSTR, ack_seq, MAC2STR(from->addr));
        inflight_seq = 0;
    }
}

// Periodic sender task: sends report, waits for ACK, retries on timeout
static void periodic_send(void* arg) {
    (void)arg;

    const uint8_t tag_dummy[6] = {0xAA,0xBB,0xCC,0x11,0x22,0x33};

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(CONFIG_KG_SEND_PERIOD_MS));
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);

        // Handle retry window
        if (inflight_seq != 0) {
            if (now_ms - inflight_at >= CONFIG_KG_ACK_TIMEOUT_MS) {
                if (inflight_try >= CONFIG_KG_ACK_RETRIES) {
                    ESP_LOGW(TAG, "seq %u give up (no ACK)", inflight_seq);
                    inflight_seq = 0; // drop it; next loop will send a new seq
                } else {
                    mesh_addr_t root;
                    if (!mesh_tr_get_root_addr(&root)) {
                        ESP_LOGW(TAG, "retry: no root addr yet");
                        continue;
                    }
                    app_report_t rpt;
                    app_build_report(&rpt, g_room, inflight_seq, g_batch, -67, tag_dummy);
                    if (mesh_tr_send(&root, &rpt, sizeof(rpt))) {
                        inflight_try++;
                        inflight_at = now_ms;
                        ESP_LOGI(TAG, "retry seq=%u (try %u)", inflight_seq, inflight_try);
                    }
                }
            }
            continue;
        }

        // New report
        mesh_addr_t root;
        if (!mesh_tr_get_root_addr(&root)) {
            ESP_LOGW(TAG, "no root addr yet");
            continue;
        }

        app_report_t rpt;
        app_build_report(&rpt, g_room, g_seq, g_batch, -67, tag_dummy);

        if (mesh_tr_send(&root, &rpt, sizeof(rpt))) {
            inflight_seq = g_seq;
            inflight_try = 1;
            inflight_at  = now_ms;
            g_seq++;
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Node starting…");

    // ---- Mesh config (explicit field assigns; no designated initializers) ----
    mesh_cfg_simple_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.mesh_id[0] = 0x4B; // 'K'
    cfg.mesh_id[1] = 0x47; // 'G'
    cfg.mesh_id[2] = 0x01;
    cfg.mesh_id[3] = 0x02;
    cfg.mesh_id[4] = 0x03;
    cfg.mesh_id[5] = 0x04;
    cfg.channel    = 6;
    strcpy(cfg.ap_pass, "keygrid123"); // 8..15 chars

    mesh_tr_init(false /*force_root*/, &cfg, on_mesh_rx);

    // Sender task pinned to core 1 (can be 0 as well)
    xTaskCreatePinnedToCore(periodic_send, "periodic_send", 4096, NULL, 5, NULL, 1);
}

#include "mesh_transport.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_mesh.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "esp_netif.h"

static const char* TAG = "mesh_tr";
static mesh_rx_cb_t s_rx_cb = NULL;
static bool         s_is_root = false;
static mesh_addr_t  s_root_addr = {0};
static mesh_cfg_simple_t s_cfg = {0};
static TaskHandle_t s_rx_task = NULL;

static void wifi_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_mesh_netifs(NULL, NULL);

    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wcfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
}

static void rx_task(void* arg) {
    (void)arg;
    while (1) {
        mesh_addr_t from = {0};
        mesh_data_t data = {0};
        int flag = 0;
        uint8_t buf[256];
        data.data = buf;
        data.size = sizeof(buf);

        if (esp_mesh_recv(&from, &data, portMAX_DELAY, &flag, NULL, 0) == ESP_OK) {
            if (s_rx_cb) {
                s_rx_cb(&from, data.data, data.size);
            }
        }
    }
}

static void mesh_event_handler(void* arg, esp_event_base_t base,
                               int32_t id, void* event_data) {
    (void)arg; (void)base;

    switch (id) {
    case MESH_EVENT_STARTED:
        ESP_LOGI(TAG, "MESH_EVENT_STARTED layer=%d", esp_mesh_get_layer());
        break;
    case MESH_EVENT_PARENT_CONNECTED: {
        mesh_event_connected_t* c = (mesh_event_connected_t*)event_data;
        ESP_LOGI(TAG, "PARENT_CONNECTED layer=%d parent=" MACSTR,
                 esp_mesh_get_layer(), MAC2STR(c->connected.bssid));
        break;
    }
    case MESH_EVENT_PARENT_DISCONNECTED:
        ESP_LOGW(TAG, "PARENT_DISCONNECTED");
        break;
    case MESH_EVENT_ROOT_ADDRESS: {
        mesh_event_root_address_t* ra = (mesh_event_root_address_t*)event_data;
        memcpy(s_root_addr.addr, ra->addr, 6);
        ESP_LOGI(TAG, "ROOT_ADDRESS " MACSTR, MAC2STR(s_root_addr.addr));
        break;
    }
    case MESH_EVENT_LAYER_CHANGE:
        ESP_LOGI(TAG, "LAYER_CHANGE -> %d", esp_mesh_get_layer());
        break;
    default:
        break;
    }
}

void mesh_tr_init(bool force_root, const mesh_cfg_simple_t* cfg, mesh_rx_cb_t on_rx) {
    s_rx_cb = on_rx;
    if (cfg) {
        memcpy(&s_cfg, cfg, sizeof(s_cfg));
    }

    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init();
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT,
                                               ESP_EVENT_ANY_ID,
                                               &mesh_event_handler,
                                               NULL));

    mesh_cfg_t mcfg = MESH_INIT_CONFIG_DEFAULT();
    memcpy(mcfg.mesh_id.id, s_cfg.mesh_id, 6);
    mcfg.channel = s_cfg.channel;

    mcfg.router.ssid_len = 0;
    memset(mcfg.router.ssid, 0, sizeof(mcfg.router.ssid));
    memset(mcfg.router.password, 0, sizeof(mcfg.router.password));

    mcfg.ap.max_connection = 10;
    mcfg.ap.nonmesh_max_connection = 0;
    mcfg.ap.password = s_cfg.ap_pass[0] ? s_cfg.ap_pass : "keygrid123";
    mcfg.crypto_funcs = &g_wifi_default_mesh_crypto_funcs;

    ESP_ERROR_CHECK(esp_mesh_set_config(&mcfg));
    ESP_ERROR_CHECK(esp_mesh_set_type(force_root ? MESH_ROOT : MESH_NODE));
    s_is_root = force_root;
    ESP_ERROR_CHECK(esp_mesh_start());

    if (s_rx_task == NULL) {
        xTaskCreatePinnedToCore(rx_task, "mesh_rx", 4096, NULL, 5, &s_rx_task, 0);
    }
}

bool mesh_tr_is_root(void) {
    return s_is_root || esp_mesh_is_root();
}

bool mesh_tr_get_root_addr(mesh_addr_t* out) {
    if (!out) return false;

    if (mesh_tr_is_root()) {
        esp_wifi_get_mac(WIFI_IF_STA, out->addr);
        return true;
    }

    if (memcmp(s_root_addr.addr, "\0\0\0\0\0\0", 6) != 0) {
        *out = s_root_addr;
        return true;
    }

    return false;
}

bool mesh_tr_send(const mesh_addr_t* to, const void* data, uint16_t len) {
    if (!to || !data || len == 0) return false;

    mesh_data_t m = {
        .data  = (uint8_t*)data,
        .size  = len,
        .proto = MESH_PROTO_BIN,
        .tos   = MESH_TOS_P2P
    };

    int flag = 0;
    esp_err_t err = esp_mesh_send((mesh_addr_t*)to, &m, flag, NULL, 0);
    return (err == ESP_OK);
}

bool mesh_tr_send_broadcast(const void* data, uint16_t len) {
    mesh_addr_t any;
    for (int i = 0; i < 6; ++i) {
        any.addr[i] = 0xFF;
    }
    return mesh_tr_send(&any, data, len);
}

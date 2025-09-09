#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_mesh.h"   // gives mesh_addr_t + APIs

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*mesh_rx_cb_t)(const mesh_addr_t* from,
                             const uint8_t* data,
                             uint16_t len);

typedef struct {
    uint8_t  mesh_id[6];
    uint8_t  channel;
    char     ap_pass[16];
    char     router_ssid[32];
    char     router_pass[64];
} mesh_cfg_simple_t;

void mesh_tr_init(bool force_root,
                  const mesh_cfg_simple_t* cfg,
                  mesh_rx_cb_t on_rx);

bool mesh_tr_is_root(void);
bool mesh_tr_get_root_addr(mesh_addr_t* out);
bool mesh_tr_send(const mesh_addr_t* to, const void* data, uint16_t len);
bool mesh_tr_send_broadcast(const void* data, uint16_t len);

#ifdef __cplusplus
}
#endif

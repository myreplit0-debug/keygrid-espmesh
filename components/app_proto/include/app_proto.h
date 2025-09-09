#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed)) {
    uint8_t  type;    // 0x01 report
    uint8_t  room;    // node's room/zone id
    uint16_t seq;     // sequence number
    uint32_t batch;   // scan batch id
    int8_t   rssi;    // strongest RSSI (dummy for now)
    uint8_t  tag[6];  // BLE MAC (dummy for now)
} app_report_t;

typedef struct __attribute__((packed)) {
    uint8_t  type;   // 0x02 ack
    uint16_t seq;    // echoes the seq
} app_ack_t;

#define APP_TYPE_REPORT 0x01
#define APP_TYPE_ACK    0x02

bool app_build_report(app_report_t* rpt,
                      uint8_t room,
                      uint16_t seq,
                      uint32_t batch,
                      int8_t rssi,
                      const uint8_t tag[6]);

bool app_parse_ack(const uint8_t* buf,
                   uint16_t len,
                   uint16_t* out_seq);

#ifdef __cplusplus
}
#endif

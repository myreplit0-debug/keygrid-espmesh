#include "app_proto.h"
#include <string.h>   // memcpy

bool app_build_report(app_report_t* rpt,
                      uint8_t room,
                      uint16_t seq,
                      uint32_t batch,
                      int8_t rssi,
                      const uint8_t tag[6]) {
    if (!rpt) return false;
    rpt->type  = APP_TYPE_REPORT;
    rpt->room  = room;
    rpt->seq   = seq;
    rpt->batch = batch;
    rpt->rssi  = rssi;
    memcpy(rpt->tag, tag, 6);
    return true;
}

bool app_parse_ack(const uint8_t* buf, uint16_t len, uint16_t* out_seq) {
    if (!buf || len < sizeof(app_ack_t)) return false;
    const app_ack_t* ack = (const app_ack_t*)buf;
    if (ack->type != APP_TYPE_ACK) return false;
    if (out_seq) *out_seq = ack->seq;
    return true;
}

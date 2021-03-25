#ifndef COM_H
#include <stdint.h>

// Command Ingest
const uint8_t COM_TYPE_CI   = 1;
// Command Response
const uint8_t COM_TYPE_CI_R = 2;
// Telemetry Output
const uint8_t COM_TYPE_TO   = 3;

const uint8_t COM_VERSION = 1;

struct COM_Msg {
    uint8_t header;
};

void COM_init(struct COM_Msg *msg, uint8_t type);
uint8_t COM_version(struct COM_Msg *msg);
uint8_t COM_type(struct COM_Msg *msg);
#endif
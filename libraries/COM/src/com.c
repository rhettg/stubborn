#include "com.h"

void COM_init(struct COM_Msg *msg, uint8_t type)
{
    msg->header = (COM_VERSION << 4 | type);
}

uint8_t COM_version(struct COM_Msg *msg) 
{
    return msg->header >> 4;
}

uint8_t COM_type(struct COM_Msg *msg)
{
    return msg->header & 0x0F;
}
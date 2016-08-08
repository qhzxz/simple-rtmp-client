//
// Created by 秦汉 on 16/7/25.
//

#ifndef SOCKET_RTMP_PACKET_HANDLER_H
#define SOCKET_RTMP_PACKET_HANDLER_H


#include "rtmp_packet.h"

class RtmpRxPacketHandler {

public:
    virtual void handleRxPacket(RtmpPacket *packet) = 0;

    virtual void notifyWindowAckRequired(const int numBytesReadThusFar)=0;

};


#endif //SOCKET_RTMP_PACKET_HANDLER_H

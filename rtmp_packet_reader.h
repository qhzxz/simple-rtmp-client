//
// Created by 秦汉 on 16/7/14.
//

#ifndef SOCKET_RTMP_PACKET_FACTORY_H
#define SOCKET_RTMP_PACKET_FACTORY_H


#include "rtmp_io.h"

class RtmpPacketReader {
private:

    RtmpPacketReader(const RtmpPacketReader &factory);

    RtmpPacketReader &operator=(const RtmpPacketReader &factory);

    int readHeaderImpl(RtmpSocket *socket, RtmpPacketHeader &header, RtmpSessionInfo *rtmpSessionInfo);

    int readExtendedTimestamp(RtmpSocket *socket, RtmpPacketHeader &header, RtmpSessionInfo *rtmpSessionInfo);

    int readBasicHeader(RtmpSocket *socket, RtmpPacketHeader &header);


public:
    RtmpPacketReader();

    int readPacketFromSocket(RtmpSocket *socket, RtmpSessionInfo *sessionInfo, RtmpPacket **packet);

};


#endif //SOCKET_RTMP_PACKET_FACTORY_H

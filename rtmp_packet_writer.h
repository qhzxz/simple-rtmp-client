//
// Created by 秦汉 on 16/7/19.
//

#ifndef SOCKET_RTMP_PACKET_WRITER_H
#define SOCKET_RTMP_PACKET_WRITER_H


#include "rtmp_io.h"

class RtmpPacketWriter {
private:


    RtmpPacketWriter(const RtmpPacketWriter &factory);

    RtmpPacketWriter &operator=(const RtmpPacketWriter &factory);

public:
    RtmpPacketWriter();

    int writePacket(RtmpPacket *packet, RtmpSocket *socket, RtmpSessionInfo *sessionInfo);

    int writePacketHeader(RtmpPacketHeader &header, RtmpSocket *socket, ChunkStreamInfo *streamInfo, ChunkType type,
                          RtmpByteArrayStream *stream);
};


#endif //SOCKET_RTMP_PACKET_WRITER_H

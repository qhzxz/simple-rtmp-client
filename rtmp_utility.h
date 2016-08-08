//
// Created by 秦汉 on 16/7/14.
//




#ifndef SOCKET_RTMP_UTILITY_H
#define SOCKET_RTMP_UTILITY_H

#include <cstdint>
#include "rtmp_type.h"
#include "rtmp_stream.h"
#include "rtmp_socket.h"

extern int32_t readUnsignedInt32(RtmpStream *stream);

extern int32_t readUnsignedInt24(RtmpStream *stream);

extern int16_t readUnsignedInt16(RtmpStream *stream);

extern int32_t toUnsignedInt32LittleEndian(RtmpStream *stream);

extern int32_t toUnsignedInt32LittleEndian(byte *bytes);

extern void writeUnsignedInt24(RtmpStream *stream, int32_t value);

extern void writeUnsignedInt16(RtmpStream *stream, int16_t value);

extern void writeUnsignedInt32(RtmpStream *stream, int32_t value);

extern void writeUnsignedInt32LittleEndian(RtmpStream *stream, int32_t value);

extern void readBytesUntilFull(RtmpStream *stream, byte *targetBuffer, int length);

extern double readDouble(RtmpStream *stream);

extern void writeDouble(RtmpStream *stream, double d);

#endif //SOCKET_RTMP_UTILITY_H

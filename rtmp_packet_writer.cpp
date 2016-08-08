//
// Created by 秦汉 on 16/7/19.
//

#include <iostream>
#include "rtmp_packet_writer.h"
#include "rtmp_utility.h"
#include "rtmp_log.h"


RtmpPacketWriter::RtmpPacketWriter() {}

RtmpPacketWriter &RtmpPacketWriter::operator=(const RtmpPacketWriter &factory) {}

RtmpPacketWriter::RtmpPacketWriter(const RtmpPacketWriter &factory) {}

int RtmpPacketWriter::writePacket(RtmpPacket *packet, RtmpSocket *socket, RtmpSessionInfo *rtmpSessionInfo) {
    int ret = RESULT_SUCCESS;
    ChunkStreamInfo *streamInfo = rtmpSessionInfo->getChunkStreamInfo(packet->getHeader().getChunkStreamId());
    streamInfo->setPrevHeaderTx(&packet->getHeader());
    int chunkSize = rtmpSessionInfo->getTxChunkSize();
    RtmpByteArrayStream header_stream(16);//16: max header and extended time stamp size

    RtmpByteArrayStream body_stream(packet->getSize());
    packet->writeBody(&body_stream);
    packet->getHeader().setPacketLength(packet->getSize());
    byte *body_position = body_stream.data();

    int remainingBytes = packet->getSize();
    while (remainingBytes > 0) {
        bool is_first_chuck_header = remainingBytes == packet->getSize();
        ret = writePacketHeader(packet->getHeader(), socket, streamInfo,
                                is_first_chuck_header ? TYPE_0_FULL : TYPE_3_RELATIVE_SINGLE_BYTE, &header_stream);
        if (ret != RESULT_SUCCESS) {
            return ret;
        }
        int send_size = (remainingBytes) < chunkSize ? remainingBytes : chunkSize;
        ret = socket->write_bytes(body_position, send_size, NULL);
        if (ret != RESULT_SUCCESS) {
            return ret;
        }
        body_position += send_size;
        remainingBytes -= send_size;
    }
    return ret;
}

int RtmpPacketWriter::writePacketHeader(RtmpPacketHeader &header, RtmpSocket *socket,
                                        ChunkStreamInfo *streamInfo, ChunkType type, RtmpByteArrayStream *stream) {
    int ret = RESULT_SUCCESS;
    // Write basic header byte
    byte chunkBasicHeaderByte = (byte) ((type << 6) | header.getChunkStreamId());
    stream->write_1byte(chunkBasicHeaderByte);
    switch (type) {
        case TYPE_0_FULL: { //  b00 = 12 byte header (full header)
            streamInfo->markDeltaTimestampTx();
            writeUnsignedInt24(stream,
                               header.getAbsoluteTimestamp() >= 0xffffff ? 0xffffff : header.getAbsoluteTimestamp());
            writeUnsignedInt24(stream, header.getPacketLength());
            stream->write_1byte(header.getMessageType());
            writeUnsignedInt32LittleEndian(stream, header.getMessageStreamId());
            if (header.getAbsoluteTimestamp() >= 0xffffff) {
                header.setExtendedTimestamp(header.getAbsoluteTimestamp());
                writeUnsignedInt32(stream, header.getExtendedTimestamp());
            }
            break;
        }
        case TYPE_1_RELATIVE_LARGE: { // b01 = 8 bytes - like type 0. not including message ID (4 last bytes)
            header.setTimestampDelta(streamInfo->markDeltaTimestampTx());
            header.setAbsoluteTimestamp(
                    streamInfo->getPrevHeaderTx()->getAbsoluteTimestamp() + header.getTimestampDelta());
            writeUnsignedInt24(stream,
                               header.getAbsoluteTimestamp() >= 0xffffff ? 0xffffff : header.getTimestampDelta());
            writeUnsignedInt24(stream, header.getPacketLength());
            stream->write_1byte(header.getMessageType());
            if (header.getAbsoluteTimestamp() >= 0xffffff) {
                header.setExtendedTimestamp(header.getAbsoluteTimestamp());
                writeUnsignedInt32(stream, header.getExtendedTimestamp());
            }
            break;
        }
        case TYPE_2_RELATIVE_TIMESTAMP_ONLY: { // b10 = 4 bytes - Basic Header and timestamp (3 bytes) are included
            header.setTimestampDelta(streamInfo->markDeltaTimestampTx());
            header.setAbsoluteTimestamp(
                    streamInfo->getPrevHeaderTx()->getAbsoluteTimestamp() + header.getTimestampDelta());
            writeUnsignedInt24(stream,
                               (header.getAbsoluteTimestamp() >= 0xffffff) ? 0xffffff : header.getTimestampDelta());
            if (header.getAbsoluteTimestamp() >= 0xffffff) {
                header.setExtendedTimestamp(header.getAbsoluteTimestamp());
                writeUnsignedInt32(stream, header.getExtendedTimestamp());
            }
            break;
        }
        case TYPE_3_RELATIVE_SINGLE_BYTE: { // b11 = 1 byte: basic header only
            header.setTimestampDelta(streamInfo->markDeltaTimestampTx());
            header.setAbsoluteTimestamp(
                    streamInfo->getPrevHeaderTx()->getAbsoluteTimestamp() + header.getTimestampDelta());
            if (header.getAbsoluteTimestamp() >= 0xffffff) {
                header.setExtendedTimestamp(header.getAbsoluteTimestamp());
                writeUnsignedInt32(stream, header.getExtendedTimestamp());
            }
            break;
        }
        default:
            return RESULT_UNKNOWN_CHUCK_HEADER_TYPE;

    }

    int position = stream->pos();
    ret = socket->write_bytes(stream->data(), position, NULL);
    RTMP_LOG_INFO("write packet header complete,chunkType:{0:02x},chunkStreamId:{1:d},chunkBasicHeaderByte:{2:02x},messageStreamId:{3:d}",
                  type, header.getChunkStreamId(), chunkBasicHeaderByte,header.getMessageStreamId());
    stream->reset();
    return ret;
}
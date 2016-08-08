//
// Created by 秦汉 on 16/7/14.
//

#include <cstddef>
#include <iostream>
#include "rtmp_packet_reader.h"
#include "rtmp_utility.h"
#include "rtmp_log.h"


RtmpPacketReader &RtmpPacketReader::operator=(const RtmpPacketReader &factory) {}

RtmpPacketReader::RtmpPacketReader(const RtmpPacketReader &factory) {}

RtmpPacketReader::RtmpPacketReader() {
}


int RtmpPacketReader::readBasicHeader(RtmpSocket *socket, RtmpPacketHeader &header) {
    int ret = RESULT_SUCCESS;
    byte basicHeaderByte = 0x00;
    if ((ret = socket->read_1byte(basicHeaderByte)) != RESULT_SUCCESS) {
        return ret;
    }
    ChunkType chunkType = ChunkType((basicHeaderByte >> 6) & 0x03);
    header.setChunkType(chunkType); // 2 most significant bits define the chunk type
    int chunkStreamId = basicHeaderByte & 0x3F;
    header.setChunkStreamId(chunkStreamId); // 6 least significant bits define chunk stream ID
    return ret;
}

int RtmpPacketReader::readHeaderImpl(RtmpSocket *socket, RtmpPacketHeader &header, RtmpSessionInfo *rtmpSessionInfo) {
    int ret = RESULT_SUCCESS;
    // Read byte 0: chunk type and chunk stream ID
    if ((ret = readBasicHeader(socket, header)) != RESULT_SUCCESS) {
        return ret;
    }
    RtmpPacketHeader *prevHeader = rtmpSessionInfo->getChunkStreamInfo(header.getChunkStreamId())->getPrevHeaderRx();
    switch (header.getChunkType()) {
        case TYPE_0_FULL: {
            RtmpByteArrayStream header_array_stream(11);//skip chuck type and chuck stream id
            if ((ret = socket->read_full_bytes(header_array_stream.data(), 11, NULL)) != RESULT_SUCCESS) {
                return ret;
            }
            //  b00 = 12 byte header (full header)
            // Read bytes 1-3: Absolute timestamp
            header.setAbsoluteTimestamp(readUnsignedInt24(&header_array_stream));
            header.setTimestampDelta(0);
            // Read bytes 4-6: Packet length
            header.setPacketLength(readUnsignedInt24(&header_array_stream));
            // Read byte 7: Message type ID
            header.setMessageType(MessageType(header_array_stream.read_1byte()));
            // Read bytes 8-11: Message stream ID (apparently little-endian order)
            byte messageStreamIdBytes[4];
            header_array_stream.read_bytes(messageStreamIdBytes, 4);
            header.setMessageStreamId(toUnsignedInt32LittleEndian(messageStreamIdBytes));
            break;
        }
        case TYPE_1_RELATIVE_LARGE: {
            RtmpByteArrayStream header_array_stream(7);//skip chuck type and chuck stream id
            if ((ret = socket->read_full_bytes(header_array_stream.data(), 7, NULL)) != RESULT_SUCCESS) {
                return ret;
            }
            // b01 = 8 bytes - like type 0. not including message stream ID (4 last bytes)
            // Read bytes 1-3: Timestamp delta
            header.setTimestampDelta(readUnsignedInt24(&header_array_stream));
            // Read bytes 4-6: Packet length
            header.setPacketLength(readUnsignedInt24(&header_array_stream));
            // Read byte 7: Message type ID
            header.setMessageType(MessageType(header_array_stream.read_1byte()));
            if (NULL != prevHeader) {
                header.setMessageStreamId(prevHeader->getMessageStreamId());
            } else {
                header.setMessageStreamId(0);
            }
            break;
        }
        case TYPE_2_RELATIVE_TIMESTAMP_ONLY: {
            if (NULL == prevHeader) {
                return RESULT_NO_PREV_HEADER;
            }
            RtmpByteArrayStream header_array_stream(3);//skip chuck type and chuck stream id
            if ((ret = socket->read_full_bytes(header_array_stream.data(), 3, NULL)) != RESULT_SUCCESS) {
                return ret;
            }
            // b10 = 4 bytes - Basic Header and timestamp (3 bytes) are included
            // Read bytes 1-3: Timestamp delta
            header.setTimestampDelta(readUnsignedInt24(&header_array_stream));
            // Read bytes 1-4: Extended timestamp delta
            header.setPacketLength(prevHeader->getPacketLength());
            header.setMessageType(prevHeader->getMessageType());
            header.setMessageStreamId(prevHeader->getMessageStreamId());
            break;
        }
        case TYPE_3_RELATIVE_SINGLE_BYTE: {
            if (NULL == prevHeader) {
                return RESULT_NO_PREV_HEADER;
            }
            header.setPacketLength(prevHeader->getPacketLength());
            header.setMessageType(prevHeader->getMessageType());
            header.setMessageStreamId(prevHeader->getMessageStreamId());
            break;
        }
        default:
            ret = RESULT_UNKNOWN_CHUCK_HEADER_TYPE;
            break;
    }

    return ret;
}

int RtmpPacketReader::readExtendedTimestamp(RtmpSocket *socket, RtmpPacketHeader &header,
                                            RtmpSessionInfo *rtmpSessionInfo) {
    int ret = RESULT_SUCCESS;
    RtmpPacketHeader *prevHeader = rtmpSessionInfo->getChunkStreamInfo(header.getChunkStreamId())->getPrevHeaderRx();
    RtmpByteArrayStream e_bytes(4);
    switch (header.getChunkType()) {
        case TYPE_0_FULL: {
//            // Read bytes 1-4: Extended timestamp
            bool is_extended = header.getAbsoluteTimestamp() >= 0xffffff;
            int extendedTimestamp = 0;
            if (is_extended) {
                if ((ret = socket->read_full_bytes(e_bytes.data(), 4, NULL)) != RESULT_SUCCESS) {
                    return ret;
                }
                extendedTimestamp = readUnsignedInt32(&e_bytes);
            }
            if (extendedTimestamp != 0) {
                header.setAbsoluteTimestamp(extendedTimestamp);
            }
            break;
        }
        case TYPE_1_RELATIVE_LARGE: {
            // Read bytes 1-4: Extended timestamp delta
            bool is_extended = header.getTimestampDelta() >= 0xffffff;
            int extendedTimestamp = 0;
            if (is_extended) {
                if ((ret = socket->read_full_bytes(e_bytes.data(), 4, NULL)) != RESULT_SUCCESS) {
                    return ret;
                }
                extendedTimestamp = readUnsignedInt32(&e_bytes);
            }
            if (NULL != prevHeader) {
                int absoluteTimestamp =
                        extendedTimestamp != 0 ? extendedTimestamp : prevHeader->getAbsoluteTimestamp() +
                                                                     header.getTimestampDelta();
                header.setAbsoluteTimestamp(absoluteTimestamp);
            } else {
                int absoluteTimestamp = extendedTimestamp != 0 ? extendedTimestamp : header.getTimestampDelta();
                header.setAbsoluteTimestamp(absoluteTimestamp);
            }
            break;
        }
        case TYPE_2_RELATIVE_TIMESTAMP_ONLY: {
            if (NULL == prevHeader) {
                return RESULT_NO_PREV_HEADER;
            }

            bool is_extended = header.getTimestampDelta() >= 0xffffff;
            int extendedTimestamp = 0;
            if (is_extended) {
                if ((ret = socket->read_full_bytes(e_bytes.data(), 4, NULL)) != RESULT_SUCCESS) {
                    return ret;
                }
                extendedTimestamp = readUnsignedInt32(&e_bytes);
            }
            int absoluteTimestamp =
                    extendedTimestamp != 0 ? extendedTimestamp : prevHeader->getAbsoluteTimestamp() +
                                                                 header.getTimestampDelta();
            header.setAbsoluteTimestamp(absoluteTimestamp);
            break;
        }
        case TYPE_3_RELATIVE_SINGLE_BYTE: {
            if (NULL == prevHeader) {
                return RESULT_NO_PREV_HEADER;
            }
            // b11 = 1 byte: basic header only
            // Read bytes 1-4: Extended timestamp
            bool is_extended = prevHeader->getTimestampDelta() >= 0xffffff;
            int extendedTimestamp = 0;
            if (is_extended) {
                if ((ret = socket->read_full_bytes(e_bytes.data(), 4, NULL)) != RESULT_SUCCESS) {
                    return ret;
                }
                extendedTimestamp = readUnsignedInt32(&e_bytes);
            }
            int timestampDelta = extendedTimestamp != 0 ? 0xffffff : prevHeader->getTimestampDelta();
            header.setTimestampDelta(timestampDelta);
            int absoluteTimestamp =
                    extendedTimestamp != 0 ? extendedTimestamp : prevHeader->getAbsoluteTimestamp() + timestampDelta;
            header.setAbsoluteTimestamp(absoluteTimestamp);
            break;
        }
        default:
            ret = RESULT_UNKNOWN_CHUCK_HEADER_TYPE;
            break;
    }

    return ret;

}

int RtmpPacketReader::readPacketFromSocket(RtmpSocket *socket, RtmpSessionInfo *rtmpSessionInfo, RtmpPacket **packet) {
    int ret = RESULT_SUCCESS;
    RtmpPacketHeader header;
    if ((ret = readHeaderImpl(socket, header, rtmpSessionInfo)) != RESULT_SUCCESS) {
        return ret;
    }
    if ((ret = readExtendedTimestamp(socket, header, rtmpSessionInfo)) != RESULT_SUCCESS) {
        return ret;
    }
    RTMP_LOG_INFO(
            "read header complete, header.chunkStreamId={0:d},header.MessageType={1:02x},header.ChunkType={2:02x},header.packerLength={3:d},header.MessageStreamId={4:d}",
            header.getChunkStreamId(), header.getMessageType(), header.getChunkType(), header.getPacketLength(),
            header.getMessageStreamId());
    RtmpPacket *temp_packet = NULL;
    ChunkStreamInfo *chunkStreamInfo = rtmpSessionInfo->getChunkStreamInfo(header.getChunkStreamId());
    chunkStreamInfo->setPrevHeaderRx(&header);
    RtmpByteArrayStream *data = NULL;
    int packetLength = header.getPacketLength();
    if (packetLength > rtmpSessionInfo->getRxChunkSize()) {
        // This packet consists of more than one chunk; store the chunks in the chunk stream until everything is read
        ret = chunkStreamInfo->storePacketChunk(socket, rtmpSessionInfo->getRxChunkSize());
        if (ret != RESULT_SUCCESS) {
            RTMP_LOG_INFO("incomplete packet");
            return ret; // packet is not yet complete or connection error
        } else {
            data = chunkStreamInfo->getStoredPacketData();
            RTMP_LOG_INFO("complete packet , size={0:d}", data->size());
        }
    } else {
        byte temp_data[packetLength];
        ret = socket->read_full_bytes(temp_data, packetLength, NULL);
        if (ret != RESULT_SUCCESS) {
            return ret;
        }
        data = new RtmpByteArrayStream(temp_data, packetLength);
        RTMP_LOG_INFO("directly read packet");
    }

    switch (header.getMessageType()) {
        case SET_CHUNK_SIZE: {
            SetChunkSize setChunkSize(header);
            setChunkSize.readBody(data);
            rtmpSessionInfo->setRxChunkSize(setChunkSize.getChunkSize());
        }
            break;
        case ABORT:
            temp_packet = new Abort(header);
            break;
        case USER_CONTROL_MESSAGE:
            temp_packet = new UserControl(header);
            break;
        case WINDOW_ACKNOWLEDGEMENT_SIZE:
            temp_packet = new WindowAckSize(header);
            break;
        case SET_PEER_BANDWIDTH:
            temp_packet = new SetPeerBandwidth(header);
            break;
        case AUDIO:
            temp_packet = new Audio(header, packetLength);
            break;
        case VIDEO:
            temp_packet = new Video(header, packetLength);
            break;
        case COMMAND_AMF0:
            temp_packet = new Command(header);
            break;
        case DATA_AMF0:
            temp_packet = new Data(header);
            break;
        case ACKNOWLEDGEMENT:
            temp_packet = new Acknowledgement(header);
            break;
        default:
            ret = RESULT_UNKNOWN_MESSAGE_TYPE;
            break;
    }
    if (NULL != temp_packet) {
        temp_packet->readBody(data);
        *packet = temp_packet;
    }
    delete data;
    data = NULL;
    return ret;
}
//
// Created by 秦汉 on 16/7/12.
//

#include <iostream>
#include "rtmp_packet.h"
#include "rtmp_utility.h"
#include "rtmp_log.h"


int RtmpPacketHeader::getChunkStreamId() {
    return chunkStreamId;
}

RtmpPacketHeader::RtmpPacketHeader() {

}

RtmpPacketHeader::RtmpPacketHeader(MessageType messageType, ChunkType chunkType, int chunkStreamId) : messageType(
        messageType),
                                                                                                      chunkType(
                                                                                                              chunkType),
                                                                                                      chunkStreamId(
                                                                                                              chunkStreamId) {}

ChunkType RtmpPacketHeader::getChunkType() {
    return chunkType;
}

int RtmpPacketHeader::getPacketLength() {
    return packetLength;
}

int RtmpPacketHeader::getMessageStreamId() {
    return messageStreamId;
}

MessageType RtmpPacketHeader::getMessageType() {
    return messageType;
}

int RtmpPacketHeader::getAbsoluteTimestamp() {
    return absoluteTimestamp;
}

void RtmpPacketHeader::setAbsoluteTimestamp(int absoluteTimestamp) {
    this->absoluteTimestamp = absoluteTimestamp;
}

int RtmpPacketHeader::getTimestampDelta() {
    return timestampDelta;
}

void RtmpPacketHeader::setTimestampDelta(int timestampDelta) {
    this->timestampDelta = timestampDelta;
}

void RtmpPacketHeader::setChunkStreamId(int channelId) {
    this->chunkStreamId = channelId;
}

void RtmpPacketHeader::setChunkType(ChunkType chunkType) {
    this->chunkType = chunkType;
}

void RtmpPacketHeader::setMessageStreamId(int messageStreamId) {
    this->messageStreamId = messageStreamId;
}

void RtmpPacketHeader::setMessageType(MessageType messageType) {
    this->messageType = messageType;
}

void RtmpPacketHeader::setPacketLength(int packetLength) {
    this->packetLength = packetLength;
}

int RtmpPacketHeader::getExtendedTimestamp() const {
    return extendedTimestamp;
}

void RtmpPacketHeader::setExtendedTimestamp(int extendedTimestamp) {
    RtmpPacketHeader::extendedTimestamp = extendedTimestamp;
}

RtmpPacket::~RtmpPacket() {}

RtmpPacket::RtmpPacket(RtmpPacketHeader header) {
    this->header = header;
}

RtmpPacketHeader &RtmpPacket::getHeader() {
    return header;
}

SetChunkSize::SetChunkSize(const RtmpPacketHeader &header) : RtmpPacket(header) {

}

SetChunkSize::SetChunkSize() : RtmpPacket(
        RtmpPacketHeader(SET_CHUNK_SIZE, TYPE_1_RELATIVE_LARGE, RTMP_CONTROL_CHANNEL)) {

}

int SetChunkSize::readBody(RtmpStream *arr) {
    chunkSize = readUnsignedInt32(arr);
}

int SetChunkSize::writeBody(RtmpStream *socket) {
    writeUnsignedInt32(socket, chunkSize);
}

int SetChunkSize::getSize() {
    return 4;
}

Abort::Abort(const RtmpPacketHeader &header) : RtmpPacket(header) {}

Abort::Abort(int chunkStreamId) : RtmpPacket(
        RtmpPacketHeader(SET_CHUNK_SIZE, TYPE_1_RELATIVE_LARGE, RTMP_CONTROL_CHANNEL)),
                                  chunkStreamId(chunkStreamId) {}

int Abort::readBody(RtmpStream *arr) {
    chunkStreamId = readUnsignedInt32(arr);
}

int Abort::writeBody(RtmpStream *stream) {
    writeUnsignedInt32(stream, chunkStreamId);
}

int Abort::getSize() {
    return 4;
}

const int &Abort::getChunkStreamId() const {
    return chunkStreamId;
}

UserControl::UserControl(const RtmpPacketHeader &header) : RtmpPacket(header) {}

UserControl::UserControl(UserControl &control, ChunkType chunkType) : RtmpPacket(
        RtmpPacketHeader(USER_CONTROL_MESSAGE, chunkType, RTMP_CONTROL_CHANNEL)), type(PONG_REPLY) {
    int size = 0;
    if (control.getType() == SET_BUFFER_LENGTH) {
        size = 2;
    } else {
        size = 1;
    }
    this->eventData = new int[size];
    memcpy(this->eventData, control.eventData, size);
}

UserControl::UserControl(Type type, ChunkType chunkType) : type(type), RtmpPacket(
        RtmpPacketHeader(USER_CONTROL_MESSAGE, chunkType, RTMP_CONTROL_CHANNEL)) {
}

UserControl::UserControl(ChunkType chunkType) : RtmpPacket(
        RtmpPacketHeader(USER_CONTROL_MESSAGE, chunkType, RTMP_CONTROL_CHANNEL)) {
}

int UserControl::readBody(RtmpStream *arr) {
    type = Type(readUnsignedInt16(arr));
    int bytesRead = 2;
    // Event data (1 for most types, 2 for SET_BUFFER_LENGTH)
    if (type == SET_BUFFER_LENGTH) {
        setEventData(readUnsignedInt32(arr), readUnsignedInt32(arr));
        bytesRead += 8;
    } else {
        setEventData(readUnsignedInt32(arr));
        bytesRead += 4;
    }
}


int UserControl::setEventData(int eventData) {
    if (type == SET_BUFFER_LENGTH) {
        return RESULT_FAIL_SET_USER_CONTROL_EVENT_DATA;
    }
    if (NULL != this->eventData) {
        delete[]this->eventData;
    }
    this->eventData = new int[1];
    this->eventData[0] = eventData;
}

int UserControl::setEventData(int streamId, int bufferLength) {
    if (type != SET_BUFFER_LENGTH) {
        return RESULT_FAIL_SET_USER_CONTROL_EVENT_DATA;
    }
    if (NULL != this->eventData) {
        delete[]this->eventData;
    }
    this->eventData = new int[2];
    this->eventData[0] = streamId;
    this->eventData[1] = bufferLength;
    return RESULT_SUCCESS;
}

UserControl::~UserControl() {
    delete[]this->eventData;
    this->eventData = NULL;
}

int UserControl::writeBody(RtmpStream *stream) {
    writeUnsignedInt16(stream, (int16_t) type);
    // Now write the event data
    writeUnsignedInt32(stream, eventData[0]);
    if (type == SET_BUFFER_LENGTH) {
        writeUnsignedInt32(stream, eventData[1]);
    }
}

int UserControl::getSize() {
    int size = 0;
    size += 2;//2 byte for type
    size += 4;//
    if (type == SET_BUFFER_LENGTH) {
        size += 4;
    }
    return size;
}

const Type &UserControl::getType() const {
    return type;
}

WindowAckSize::WindowAckSize(const RtmpPacketHeader &header) : RtmpPacket(header) {}

WindowAckSize::WindowAckSize(int acknowledgementWindowSize, ChunkType type) : RtmpPacket(
        RtmpPacketHeader(WINDOW_ACKNOWLEDGEMENT_SIZE, type, RTMP_CONTROL_CHANNEL)), acknowledgementWindowSize(
        acknowledgementWindowSize) {

}

int WindowAckSize::getAcknowledgementWindowSize() const {
    return acknowledgementWindowSize;
}

void WindowAckSize::setAcknowledgementWindowSize(int acknowledgementWindowSize) {
    WindowAckSize::acknowledgementWindowSize = acknowledgementWindowSize;
}

int WindowAckSize::readBody(RtmpStream *stream) {
    acknowledgementWindowSize = readUnsignedInt32(stream);
}

int WindowAckSize::writeBody(RtmpStream *stream) {
    writeUnsignedInt32(stream, acknowledgementWindowSize);
}

int WindowAckSize::getSize() {
    return 4;
}

SetPeerBandwidth::SetPeerBandwidth(const RtmpPacketHeader &header) : RtmpPacket(header) {}

SetPeerBandwidth::SetPeerBandwidth(int acknowledgementWindowSize, ChunkType type, LimitType l_type) : RtmpPacket(
        RtmpPacketHeader(WINDOW_ACKNOWLEDGEMENT_SIZE, type, RTMP_CONTROL_CHANNEL)), acknowledgementWindowSize(
        acknowledgementWindowSize), limitType(l_type) {

}


int SetPeerBandwidth::getAcknowledgementWindowSize() const {
    return acknowledgementWindowSize;
}

void SetPeerBandwidth::setAcknowledgementWindowSize(int acknowledgementWindowSize) {
    SetPeerBandwidth::acknowledgementWindowSize = acknowledgementWindowSize;
}

LimitType SetPeerBandwidth::getLimitType() const {
    return limitType;
}

void SetPeerBandwidth::setLimitType(LimitType limitType) {
    SetPeerBandwidth::limitType = limitType;
}

int SetPeerBandwidth::readBody(RtmpStream *stream) {
    acknowledgementWindowSize = readUnsignedInt32(stream);
    limitType = LimitType((int) stream->read_1byte());
}

int SetPeerBandwidth::writeBody(RtmpStream *stream) {
    writeUnsignedInt32(stream, acknowledgementWindowSize);
    stream->write_1byte((byte) limitType);
}

int SetPeerBandwidth::getSize() {
    return 5;
}

ContentData::ContentData(const RtmpPacketHeader &header) : RtmpPacket(header) {
    data = NULL;
}

void ContentData::setData(byte *data, int size) {
    this->size = size;
    if (NULL != this->data) {
        delete[] this->data;
    }
    this->data = new byte[size];
    this->getHeader().setPacketLength(size);
    memcpy(this->data, data, size);
}

byte *ContentData::getData() { return data; }

ContentData::~ContentData() {
    delete[]data;
    data = NULL;
}

int ContentData::readBody(RtmpStream *stream) {
    if (NULL != data) {
        delete[]data;
    }
    data = new byte[size];
    stream->read_bytes(data, size);
}

int ContentData::writeBody(RtmpStream *stream) {
    stream->write_bytes(data, size);
}

int ContentData::getSize() {
    return size;
}

Audio::Audio(const RtmpPacketHeader &header) : ContentData(header) {}

Audio::Audio() : ContentData(RtmpPacketHeader(AUDIO, TYPE_0_FULL, RTMP_AUDIO_CHANNEL)) {}

Video::Video(const RtmpPacketHeader &header) : ContentData(header) {}

Video::Video() : ContentData(RtmpPacketHeader(VIDEO, TYPE_0_FULL, RTMP_VIDEO_CHANNEL)) {}

VariableBodyRtmpPacket::VariableBodyRtmpPacket(const RtmpPacketHeader &header) : RtmpPacket(header) {
    items = new std::vector<AmfData *>;
};

void VariableBodyRtmpPacket::addAmfData(AmfData *dataItem) {
    if (NULL != dataItem) {
        items->push_back(dataItem);
    } else {
        items->push_back(new AmfNull);
    }
}

void VariableBodyRtmpPacket::addBoolData(bool flag) {
    addAmfData(new AmfBoolean(flag));
}

void VariableBodyRtmpPacket::addDoubleData(double number) {
    addAmfData(new AmfNumber(number));
}

void VariableBodyRtmpPacket::addStringData(std::string str) {
    addAmfData(new AmfString(str));
}

void VariableBodyRtmpPacket::readVariableData(RtmpStream *in, int bytesAlreadyRead) {
    do {
        AmfData *dataItem = AmfDecoder::decode(in);
        addAmfData(dataItem);
        bytesAlreadyRead += dataItem->getSize();
    } while (bytesAlreadyRead < getHeader().getPacketLength());
}

void VariableBodyRtmpPacket::writeVariableData(RtmpStream *in) {
    if (!items->empty()) {
        for (auto begin = items->begin(); begin != items->end(); ++begin) {
            AmfData *data = (*begin);
            data->writeTo(in);
        }
    } else {
        // Write a null
        AmfNull::writeNullTo(in);
    }
}

VariableBodyRtmpPacket::~VariableBodyRtmpPacket() {
    for (auto begin = items->begin(); begin != items->end(); ++begin) {
        AmfData *data = (*begin);
        delete data;
        data = NULL;
    }
    items->clear();
    items = NULL;
}


int VariableBodyRtmpPacket::getSize() {
    int size = 0;
    if (!items->empty()) {
        for (auto begin = items->begin(); begin != items->end(); ++begin) {
            AmfData *data = (*begin);
            size += data->getSize();
        }
    } else {
        // Write a null
        size += 1;
    }
    return size;
}

std::vector<AmfData *> *VariableBodyRtmpPacket::getItems() { return items; }


Command::Command(const RtmpPacketHeader &header) : VariableBodyRtmpPacket(header) {}

Command::Command(std::string commandName, int transactionId) :
        VariableBodyRtmpPacket(RtmpPacketHeader(COMMAND_AMF0, TYPE_0_FULL, RTMP_COMMAND_CHANNEL)),
        transactionId(transactionId),
        commandName(commandName) {}

Command::Command(std::string commandName, int transactionId, ChunkType type) :
        VariableBodyRtmpPacket(RtmpPacketHeader(COMMAND_AMF0, type, RTMP_COMMAND_CHANNEL)),
        transactionId(transactionId),
        commandName(commandName) {

}

int Command::readBody(RtmpStream *stream) {
    AmfType type = AmfType(stream->read_1byte());// Read past the data type byte
    if (AMF0_STRING != type) {
        RTMP_LOG_INFO("Command body read type not AmfString ,type={0:x}", type);
    }
    int length = readUnsignedInt16(stream);
    byte data[length];
    readBytesUntilFull(stream, data, length);
    commandName = std::string((char *) data, length);
    transactionId = AmfNumber::readNumberFrom(stream);
    int bytesAlreadyRead = (1 + 2 + length) + AmfNumber::SIZE;
    readVariableData(stream, bytesAlreadyRead);
}

int Command::writeBody(RtmpStream *stream) {
    stream->write_1byte(AMF0_STRING);
    writeUnsignedInt16(stream, commandName.length());
    stream->write_bytes((byte *) commandName.data(), commandName.length());
    stream->write_1byte(AMF0_NUMBER);
    writeDouble(stream, transactionId);
    writeVariableData(stream);
}

int Command::getSize() {
    int size = 0;
    size += 1;
    size += 2;
    size += commandName.length();
    size += 1;
    size += 8;
    size += VariableBodyRtmpPacket::getSize();
    return size;
}

const std::string &Command::getCommandName() const {
    return commandName;
}

int Command::getTransactionId() const {
    return transactionId;
}


const std::string &Data::getType() { return type; }

Data::Data(const RtmpPacketHeader &header) : VariableBodyRtmpPacket(header) {}

Data::Data(std::string type) : VariableBodyRtmpPacket(RtmpPacketHeader(DATA_AMF0, TYPE_0_FULL, RTMP_COMMAND_CHANNEL)),
                               type(type) {}

void Data::setType(std::string type) {
    this->type = type;
}

int Data::readBody(RtmpStream *in) {
    // Read notification type
    in->read_1byte();// Read past the data type byte
    int length = readUnsignedInt16(in);
    byte data[length];
    readBytesUntilFull(in, data, length);
    type = std::string((char *) data, length);
    int bytesRead = (1 + 2 + length);
    // Read data body
    readVariableData(in, bytesRead);
}


int Data::writeBody(RtmpStream *stream) {
    stream->write_1byte(AMF0_STRING);
    writeUnsignedInt16(stream, type.length());
    stream->write_bytes((byte *) type.data(), type.length());
    writeVariableData(stream);
}

int Data::getSize() {
    int size = 0;
    size += 1;
    size += 2;
    size += type.length();
    size += VariableBodyRtmpPacket::getSize();
    return size;
}

Acknowledgement::Acknowledgement(const RtmpPacketHeader &header) : RtmpPacket(header) {}

Acknowledgement::Acknowledgement(int numBytesReadThusFar) :
        RtmpPacket(RtmpPacketHeader(ACKNOWLEDGEMENT, TYPE_0_FULL, RTMP_CONTROL_CHANNEL)),
        sequenceNumber(numBytesReadThusFar) {}

int Acknowledgement::getSequenceNumber() { return sequenceNumber; }

void Acknowledgement::setSequenceNumber(int numBytesRead) {
    this->sequenceNumber = numBytesRead;
}

int Acknowledgement::readBody(RtmpStream *in) {
    sequenceNumber = readUnsignedInt32(in);
}

int Acknowledgement::writeBody(RtmpStream *out) {
    writeUnsignedInt32(out, sequenceNumber);
}

int Acknowledgement::getSize() {
    return 4;
}
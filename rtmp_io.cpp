//
// Created by 秦汉 on 16/7/12.
//

#include "rtmp_io.h"
#include "rtmp_log.h"


ChunkStreamInfo::ChunkStreamInfo() {
    baos.reserve(1024 * 128);
}

ChunkStreamInfo *RtmpSessionInfo::getChunkStreamInfo(int chunkStreamId) {
    ChunkChannelsMap::iterator it = chunkChannels.find(chunkStreamId);
    ChunkStreamInfo *chunkStreamInfo = NULL;
    if (it == chunkChannels.end()) {
        chunkStreamInfo = new ChunkStreamInfo();
        ChunkPair pair(chunkStreamId, chunkStreamInfo);
        chunkChannels.insert(pair);
    } else {
        chunkStreamInfo = (*it).second;
    }
    return chunkStreamInfo;
}


bool ChunkStreamInfo::canReusePrevHeaderTx(MessageType forMessageType) {
    return (prevHeaderTx != NULL && prevHeaderTx->getMessageType() == forMessageType);
}

ChunkStreamInfo::~ChunkStreamInfo() {
    if (NULL != prevHeaderTx) {
        delete prevHeaderTx;
    }
    if (NULL != prevHeaderRx) {
        delete prevHeaderRx;
    }
}

long ChunkStreamInfo::sessionBeginTimestamp=0;

void ChunkStreamInfo::markSessionTimestampTx() {
//        sessionBeginTimestamp = System.nanoTime() / 1000000;
}

long ChunkStreamInfo::markDeltaTimestampTx() {
//    long currentTimestamp = 1000000;
//    long diffTimestamp = currentTimestamp - realLastTimestamp;
//    ChunkStreamInfo::realLastTimestamp = currentTimestamp;
//    return diffTimestamp;
    return 0;
}

long ChunkStreamInfo::markAbsoluteTimestampTx() {
    return 1000000 - sessionBeginTimestamp;
}

void ChunkStreamInfo::setPrevHeaderTx(RtmpPacketHeader *prevHeaderTx) {
    if (NULL != this->prevHeaderTx) {
        delete this->prevHeaderTx;
    }
    this->prevHeaderTx = new RtmpPacketHeader(*prevHeaderTx);
}

std::string RtmpSessionInfo::takeInvokedCommand(int transactionId) {
    std::unique_lock<std::mutex> lck(methodLock);
    MethodMap::iterator it;
    it = invokedMethods.find(transactionId);
    bool find_result = it != invokedMethods.end();
    if (find_result) {
        invokedMethods.erase(it);
    }
    return find_result ? (*it).second : "";
}

std::string RtmpSessionInfo::addInvokedCommand(int transactionId, std::string commandName) {
    std::string preCommandName = "NULL";
    std::unique_lock<std::mutex> lck(methodLock);
    MethodMap::iterator it;
    it = invokedMethods.find(transactionId);
    if (it != invokedMethods.end()) {
        preCommandName = (*it).second;
    }
    MethodPair pair(transactionId, commandName);
    invokedMethods.insert(pair);
    return preCommandName;
}

int ChunkStreamInfo::storePacketChunk(RtmpSocket *socket, int chunkSize) {
    int ret = RESULT_SUCCESS;
    const int remainingBytes = prevHeaderRx->getPacketLength() - baos.size();
    int read_bytes_count = remainingBytes < chunkSize ? remainingBytes : chunkSize;
    byte chunk[read_bytes_count];
    if ((ret = socket->read_full_bytes(chunk, read_bytes_count, NULL)) != RESULT_SUCCESS) { return ret; }
    baos.insert(baos.end(), chunk, chunk + read_bytes_count);
    if (baos.size() < prevHeaderRx->getPacketLength()) {
        ret = RESULT_PACKET_NOT_YET_COMPLETE;
    }
    return ret;
}

RtmpByteArrayStream *ChunkStreamInfo::getStoredPacketData() {
    RtmpByteArrayStream *stream = new RtmpByteArrayStream(baos.data(), baos.size());
    baos.clear();
    return stream;
}

void ChunkStreamInfo::clearStoredChunks() {
    baos.clear();
}

RtmpPacketHeader *ChunkStreamInfo::getPrevHeaderRx() {
    return prevHeaderRx;
}

void ChunkStreamInfo::setPrevHeaderRx(RtmpPacketHeader *previousHeader) {
    if (NULL != this->prevHeaderRx) {
        delete this->prevHeaderRx;
    }
    this->prevHeaderRx = new RtmpPacketHeader(*previousHeader);
}

RtmpPacketHeader *ChunkStreamInfo::getPrevHeaderTx() {
    return prevHeaderTx;
}


bool RtmpSessionInfo::addToWindowBytesRead(const int numBytes, const RtmpPacket packet) {
    windowBytesRead += numBytes;
    totalBytesRead += numBytes;
    if (windowBytesRead >= acknowledgementWindowSize) {
        windowBytesRead -= acknowledgementWindowSize;
        return true;
    }
    return false;
}

void RtmpSessionInfo::setAcknowledgmentWindowSize(int acknowledgementWindowSize) {
    this->acknowledgementWindowSize = acknowledgementWindowSize;
}

int RtmpSessionInfo::getRxChunkSize() {
    return rxChunkSize;
}

void RtmpSessionInfo::setRxChunkSize(int chunkSize) {
    this->rxChunkSize = chunkSize;
}

int RtmpSessionInfo::getTxChunkSize() {
    return txChunkSize;
}

void RtmpSessionInfo::setTxChunkSize(int chunkSize) {
    this->txChunkSize = chunkSize;
}

int RtmpSessionInfo::getAcknowledgementWindowSize() {
    return acknowledgementWindowSize;
}

RtmpSessionInfo::~RtmpSessionInfo() {
    for (auto mapIterator = chunkChannels.begin(); mapIterator != chunkChannels.end(); ++mapIterator) {
        ChunkStreamInfo *chunkStreamInfo = (*mapIterator).second;
        if (chunkStreamInfo != NULL) {
            delete chunkStreamInfo;
            chunkStreamInfo = NULL;
        }
    }
    chunkChannels.clear();
    methodLock.unlock();
}
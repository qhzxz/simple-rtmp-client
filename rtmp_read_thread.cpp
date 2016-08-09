//
// Created by 秦汉 on 16/7/21.
//

#include <assert.h>
#include <iostream>
#include "rtmp_read_thread.h"
#include "rtmp_log.h"

RtmpReadThread::RtmpReadThread(bool isJoin, RtmpSessionInfo *sessionInfo, RtmpSocket *socket) : isJoin(isJoin),
                                                                                                sessionInfo(
                                                                                                        sessionInfo),
                                                                                                socket(socket) {
    assert(sessionInfo != NULL);
    packetReader = new RtmpPacketReader();
}

void RtmpReadThread::start() {
    stop();
    if (p_thread != NULL) {
        delete p_thread;
    }
    p_thread = new std::thread(&RtmpReadThread::readPacket, this);
    active = true;
    if (p_thread->joinable()) {
        if (isJoin) {
            p_thread->join();
        } else {
            p_thread->detach();
        }
    }
}

void RtmpReadThread::stop() {
    if (active) {
        active = false;
        std::unique_lock<std::mutex> lck(exit_lock);
        cv.wait(lck);
    }
};


void RtmpReadThread::readPacket() {
    while (active) {
        RtmpPacket *packet = NULL;
        int ret = packetReader->readPacketFromSocket(socket, sessionInfo, &packet);
        if (ret == RESULT_SUCCESS) {
            if (NULL != handler && NULL != packet) {
                handler->handleRxPacket(packet);
            }
        } else if (ret == RESULT_PACKET_NOT_YET_COMPLETE |
                   ret == RESULT_UNKNOWN_MESSAGE_TYPE |
                   ret == RESULT_UNKNOWN_CHUCK_HEADER_TYPE) {
            RTMP_LOG_INFO("packet unknown or incomplete ret={0:d}", ret);
            continue;
        } else {
            RTMP_LOG_INFO("read packet error ret={0:d}", ret);
            active = false;
            break;
        }
    }
    std::unique_lock<std::mutex> lck(exit_lock);
    cv.notify_all();
}

RtmpReadThread::~RtmpReadThread() {
    stop();
    if (NULL != p_thread) {
        delete p_thread;
    }
    if (NULL != packetReader) {
        delete packetReader;
    }
}

void RtmpReadThread::setHandler(RtmpRxPacketHandler *handler) {
    RtmpReadThread::handler = handler;
}

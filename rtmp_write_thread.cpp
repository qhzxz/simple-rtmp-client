//
// Created by 秦汉 on 16/7/22.
//

#include <iostream>
#include "rtmp_write_thread.h"
#include "rtmp_log.h"

RtmpWriteThread &RtmpWriteThread::operator=(const RtmpWriteThread &writeThread) {}

RtmpWriteThread::RtmpWriteThread(const RtmpWriteThread &writeThread) {}

RtmpWriteThread::RtmpWriteThread(bool isJoin, RtmpSessionInfo *sessionInfo, RtmpSocket *socket) : isJoin(isJoin),
                                                                                                  socket(socket),
                                                                                                  sessionInfo(
                                                                                                          sessionInfo) {
    packetWriter = new RtmpPacketWriter();
}

void RtmpWriteThread::start() {
    stop();
    if (NULL != p_thread) {
        delete p_thread;
    }
    active = true;
    p_thread = new std::thread(&RtmpWriteThread::writePacket, this);
    if (p_thread->joinable()) {
        if (isJoin) {
            p_thread->join();
        } else {
            p_thread->detach();
        }
    }
}

void RtmpWriteThread::stop() {
    if (active) {
        active = false;
        std::unique_lock<std::mutex> lck(exit_lock);
        cv.wait(lck);
    }
}

void RtmpWriteThread::writePacket() {
    while (active) {
        while (!packet_deque.empty()) {
            vector_lock.lock();
            RtmpPacket *packet = packet_deque.front();
            packet_deque.pop_front();
            vector_lock.unlock();
            int ret = packetWriter->writePacket(packet, socket, sessionInfo);
            if (ret != RESULT_SUCCESS) {
                RTMP_LOG_INFO("write packet fail ret:{0:d}", ret);
                active = false;
                break;
            } else {
                RTMP_LOG_INFO("write packet successs packet.MessageType:{0:02x},packet.Length:{1:d}",
                              packet->getHeader().getMessageType(), packet->getHeader().getPacketLength());
                Command *command = dynamic_cast<Command *>(packet);
                if (NULL != command) {
                    sessionInfo->addInvokedCommand(command->getTransactionId(), command->getCommandName());
                }
            }
            delete packet;
            packet = NULL;
        }
    }
    std::unique_lock<std::mutex> lck(exit_lock);
    cv.notify_all();
}

RtmpWriteThread::~RtmpWriteThread() {
    stop();
    if (NULL != p_thread) {
        delete p_thread;
    }

    if (NULL != packetWriter) {
        delete packetWriter;
    }
    clear();
}

void RtmpWriteThread::send(RtmpPacket *packet) {
    std::unique_lock<std::mutex> lck(vector_lock);
    packet_deque.push_back(packet);
}

void RtmpWriteThread::clear() {
    for (auto dequeIterator = packet_deque.begin(); dequeIterator != packet_deque.end(); ++dequeIterator) {
        RtmpPacket *packet = *dequeIterator;
        delete packet;
        packet = NULL;
    }
    packet_deque.clear();
}
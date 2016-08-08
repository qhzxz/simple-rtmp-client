//
// Created by 秦汉 on 16/7/22.
//

#ifndef SOCKET_RTMP_WRITE_THREAD_H
#define SOCKET_RTMP_WRITE_THREAD_H

#include <thread>
#include <deque>
#include "rtmp_packet_writer.h"

class RtmpWriteThread {
private:
    bool isJoin = false;
    volatile bool active = false;
    std::deque<RtmpPacket *> packet_deque;

    RtmpPacketWriter *packetWriter = NULL;
    RtmpSocket *socket = NULL;
    RtmpSessionInfo *sessionInfo = NULL;

    std::thread *p_thread = NULL;
    std::mutex exit_lock;
    std::mutex vector_lock;
    std::condition_variable cv;


    RtmpWriteThread(const RtmpWriteThread &writeThread);

    RtmpWriteThread &operator=(const RtmpWriteThread &writeThread);

    void writePacket();

    void clear();

public:

    RtmpWriteThread(bool isJoin, RtmpSessionInfo *sessionInfo, RtmpSocket *socket);

    void start();

    void stop();

    void send(RtmpPacket *packet);

    ~RtmpWriteThread();

};


#endif //SOCKET_RTMP_WRITE_THREAD_H

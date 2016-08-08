//
// Created by 秦汉 on 16/7/21.
//

#ifndef SOCKET_RTMP_READ_THREAD_H
#define SOCKET_RTMP_READ_THREAD_H

#include <thread>
#include "rtmp_packet_reader.h"
#include "rtmp_packet_handler.h"

class RtmpReadThread {
private:
    bool isJoin = false;
    volatile bool active = false;


    RtmpPacketReader *packetReader = NULL;
    RtmpSocket *socket = NULL;
    RtmpRxPacketHandler *handler = NULL;
    RtmpSessionInfo *sessionInfo = NULL;

    std::thread *p_thread = NULL;
    std::mutex exit_lock;
    std::condition_variable cv;

private:
    void readPacket();

    RtmpReadThread(const RtmpReadThread &p);

    RtmpReadThread &operator=(const RtmpReadThread &p);


public:
    RtmpReadThread(bool isJoin, RtmpSessionInfo *sessionInfo, RtmpSocket *socket);

    void setHandler(RtmpRxPacketHandler *handler);

    void start();

    void stop();

    ~RtmpReadThread();


};


#endif //SOCKET_RTMP_READ_THREAD_H

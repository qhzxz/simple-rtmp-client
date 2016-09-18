//
// Created by 秦汉 on 16/7/29.
//

#ifndef SOCKET_RTMP_CONNECTION_H
#define SOCKET_RTMP_CONNECTION_H


#include <regex>
#include <string>
#include <atomic>
#include "rtmp_socket.h"
#include "rtmp_io.h"
#include "rtmp_write_thread.h"
#include "rtmp_read_thread.h"
#include "rtmp_packet_handler.h"

class RtmpWriteThread;

class RtmpConnection : public RtmpRxPacketHandler {
private:
    std::string appName;
    std::string streamName;
    std::string publishType;
    std::string swfUrl;
    std::string tcUrl;
    std::string pageUrl;
    RtmpSocket *rtmpSocket = NULL;
    RtmpSessionInfo *sessionInfo = NULL;
    RtmpReadThread *rtmpReadThread = NULL;
    RtmpWriteThread *rtmpWriteThread = NULL;
    volatile bool active = false;
    volatile bool connecting = false;
    volatile bool fullyConnected = false;
    volatile bool publishPermitted = false;
    std::mutex connectLock;
    std::condition_variable cv_connect;
    std::mutex createStreamLock;
    std::condition_variable cv_stream;
    int currentStreamId = -1;
    int transactionIdCounter = 0;
    std::mutex dequeLock;
    std::deque<RtmpPacket *> rxPacketDeque;
    std::atomic_int videoFrameCacheNumber;


    void reset();

    void clearPacket();

    int handShake();

    int checkUrlPattern(std::regex &parrten, std::match_results<std::string::const_iterator> &results,
                        const std::string &url);

    int onMetaData();

    int fmlePublish();

    int createStream();

    int rtmpConnect();

public:
    RtmpConnection();


    int connect(const std::string &url);

    int closeStream();

    int publish(std::string type);

    void publishVideoData(int size, int32_t time_stamp, byte *data);

    void publishAudioData(int size, int32_t time_stamp, byte *data);

    ~RtmpConnection();

    void handleRxPacket(RtmpPacket *packet);

    int handleRxInvoke(RtmpPacket *packet);

    void handleRxPacketLoop();

    void shutdown();

    std::atomic_int &getVideoFrameCacheNumber();

    void notifyWindowAckRequired(const int numBytesReadThusFar);

    int play();
};


#endif //SOCKET_RTMP_CONNECTION_H

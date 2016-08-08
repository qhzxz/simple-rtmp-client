//
// Created by 秦汉 on 16/7/14.
//



#ifndef SOCKET_RTMP_SOCKET_H
#define SOCKET_RTMP_SOCKET_H

#include "rtmp_type.h"
#include "rtmp_stream.h"

#include <netdb.h>
#include <arpa/inet.h>
#include <string>
#include <unistd.h>


class RtmpSocket {
private:
    struct sockaddr_in *s_in = NULL;
    int s_fd;

    RtmpSocket(const RtmpSocket &rtmpSocket) {}

    RtmpSocket &operator=(const RtmpSocket &rtmpSocket) {}

public:

    RtmpSocket(std::string ip, int port);

    int setReadTimeOut(int time);

    int setWriteTimeOut(int time);

    int setSocketSendBufferSize(int size);

    int connect();



    virtual int read_1byte(byte &temp);

    virtual int read_bytes(byte *data, int size, size_t *nread);

    virtual int read_full_bytes(byte *data, int size, size_t *nread);

    virtual int write_1byte(byte value);

    virtual int write_bytes(byte *data, int size, size_t *nwrite);

    ~RtmpSocket();

    int closeSocket();
};


#endif //SOCKET_RTMP_SOCKET_H

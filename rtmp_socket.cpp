//
// Created by 秦汉 on 16/7/14.
//

#include <errno.h>
#include "rtmp_socket.h"
#include "rtmp_log.h"


RtmpSocket::RtmpSocket(std::string ip, int port) {
    s_in = new struct sockaddr_in;
    memset(s_in, 0, sizeof(*s_in));
    s_in->sin_family = AF_INET;
    s_in->sin_port = htons(port);
    s_in->sin_addr.s_addr = inet_addr(ip.c_str());
    s_fd = socket(AF_INET, SOCK_STREAM, 0);
    setReadTimeOut(1000);
    setWriteTimeOut(1000);
}


int RtmpSocket::setWriteTimeOut(int time) {
    int ret = RESULT_SUCCESS;
    struct timeval timeout = {time, 0};
    int temp = ::setsockopt(s_fd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval));
    if (temp == -1) {
        return RESULT_SET_SOCKET_WRITE_TIME_FAIL;
    }
    return ret;
}


int RtmpSocket::setReadTimeOut(int time) {
    int ret = RESULT_SUCCESS;
    struct timeval timeout = {time, 0};
    int temp = ::setsockopt(s_fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));
    if (temp == -1) {
        return RESULT_SET_SOCKET_READ_TIME_FAIL;
    }
    return ret;
}

int RtmpSocket::setSocketSendBufferSize(int size) {
    int ret = RESULT_SUCCESS;
    int temp = ::setsockopt(s_fd, SOL_SOCKET, SO_SNDBUF, (const char *) &size, sizeof(int));
    if (temp == -1) {
        return RESULT_SET_SOCKET_SEND_BUFFER_SIZE_FAIL;
    }
    return ret;
}

int RtmpSocket::connect() {
    int result = RESULT_SUCCESS;
    int temp = ::connect(s_fd, (struct sockaddr *) s_in, sizeof(struct sockaddr));
    if (temp < 0) {
        return RESULT_NOT_CONNECT;
    }
    return result;
}

int RtmpSocket::read_1byte(byte &temp) {
    int result = read_bytes(&temp, 1, NULL);
    return result;
}

int RtmpSocket::read_bytes(byte *data, int size, size_t *nread) {
    int ret = RESULT_SUCCESS;
    size_t count = ::read(s_fd, data, size);
    if (NULL != nread) {
        *nread = count;
    }
    if (count <= 0) {
        if (count < 0 && errno == ETIME) {
            return RESULT_SOCKET_TIME_OUT;
        }
        if (count == 0) {
            return RESULT_SOCKET_END;
        }
    }
    return ret;
}

int RtmpSocket::write_1byte(byte temp) {
    int result = write_bytes(&temp, 1, NULL);
    return result;
}

int RtmpSocket::write_bytes(byte *data, int size, size_t *nwrite) {
    int ret = RESULT_SUCCESS;
    size_t count = ::write(s_fd, data, size);
    if (NULL != nwrite) {
        *nwrite = count;
    }
    if (count <= 0) {
        if (count < 0 && errno == ETIME) {
            return RESULT_SOCKET_TIME_OUT;
        }
    }
    return ret;
}

int RtmpSocket::read_full_bytes(byte *data, int size, size_t *nread) {
    int ret = RESULT_SUCCESS;
    int totalBytesRead = 0;
    do {
        size_t count = ::read(s_fd, data + totalBytesRead, size - totalBytesRead);
        totalBytesRead += count;
        if (count <= 0) {
            if (count < 0 && errno == ETIME) {
                return RESULT_SOCKET_TIME_OUT;
            }
            if (count == 0) {
                return RESULT_SOCKET_END;
            }
        }
    } while (totalBytesRead < size);
    if (NULL != nread) {
        *nread = size;
    }
    return ret;
}


int RtmpSocket::closeSocket() {
    int ret = RESULT_SUCCESS;
    int temp = close(s_fd);
    if (temp < 0) {
        return RESULT_SOCKET_CLOSE_FAIL;
    }
    return ret;
};


RtmpSocket::~RtmpSocket() {
    delete s_in;
}
//
// Created by 秦汉 on 16/7/15.
//

#ifndef SOCKET_RTMP_STREAM_H
#define SOCKET_RTMP_STREAM_H


#include "rtmp_type.h"


class RtmpStream {
public:

    virtual byte read_1byte() = 0;

    /**
    * get bytes from stream, length specifies by param len.
    */
    virtual int read_bytes(byte *data, int size) = 0;

public:

    virtual int write_1byte(byte value) = 0;

    virtual int write_bytes(byte *data, int size) = 0;
};

class RtmpByteArrayStream : public RtmpStream {
private:
    byte *current_position_pointer;
    byte *bytes;
    int len;

    RtmpByteArrayStream &operator=(const RtmpByteArrayStream &s) {};

    RtmpByteArrayStream(const RtmpByteArrayStream &s) {};


public:
    RtmpByteArrayStream(int len);

    RtmpByteArrayStream(byte *data, int len);

public:
    virtual ~RtmpByteArrayStream();

    /**
    * get data of stream, set by initialize.
    * current bytes = data() + pos()
    */
    virtual byte *data();

    /**
    * the total stream size, set by initialize.
    * left bytes = size() - pos().
    */
    virtual int size();

    /**
    * tell the current pos.
    */
    virtual int pos();

    /**
    * whether stream is empty.
    * if empty, user should never read or write.
    */
    virtual bool empty();

    virtual void reset();


// to change stream.
public:
    /**
    * to skip some size.
    * @param size can be any value. positive to forward; nagetive to backward.
    * @remark to skip(pos()) to reset stream.
    * @remark assert initialized, the data() not NULL.
    */
    virtual void skip(int size);

public:

    virtual byte read_1byte();

    /**
    * get bytes from stream, length specifies by param len.
    */
    virtual int read_bytes(byte *data, int size);

public:

    virtual int write_1byte(byte value);

    virtual int write_bytes(byte *data, int size);

    bool require(int size);
};

#endif //SOCKET_RTMP_STREAM_H

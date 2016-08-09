//
// Created by 秦汉 on 16/7/12.
//

#ifndef SOCKET_RTMP_PACKET_H
#define SOCKET_RTMP_PACKET_H


#include <vector>
#include "rtmp_socket.h"
#include "rtmp_type.h"
#include "rtmp_amf0.h"

enum MessageType {

    /**
     * Protocol control message 1
     * Set Chunk Size, is used to notify the peer a new maximum chunk size to use.
     */
            SET_CHUNK_SIZE = 0x01,
    /**
     * Protocol control message 2
     * Abort Message, is used to notify the peer if it is waiting for chunks
     * to complete a message, then to discard the partially received message
     * over a chunk stream and abort processing of that message.
     */
            ABORT = 0x02,
    /**
     * Protocol control message 3
     * The client or the server sends the acknowledgment to the peer after
     * receiving bytes equal to the window size. The window size is the
     * maximum number of bytes that the sender sends without receiving
     * acknowledgment from the receiver.
     */
            ACKNOWLEDGEMENT = 0x03,
    /**
     * Protocol control message 4
     * The client or the server sends this message to notify the peer about
     * the user control events. This message carries Event type and Event
     * data.
     * Also known as a PING message in some RTMP implementations.
     */
            USER_CONTROL_MESSAGE = 0x04,
    /**
     * Protocol control message 5
     * The client or the server sends this message to inform the peer which
     * window size to use when sending acknowledgment.
     * Also known as ServerBW ("server bandwidth") in some RTMP implementations.
     */
            WINDOW_ACKNOWLEDGEMENT_SIZE = 0x05,
    /**
     * Protocol control message 6
     * The client or the server sends this message to update the output
     * bandwidth of the peer. The output bandwidth value is the same as the
     * window size for the peer.
     * Also known as ClientBW ("client bandwidth") in some RTMP implementations.
     */
            SET_PEER_BANDWIDTH = 0x06,
    /**
     * RTMP audio packet (0x08)
     * The client or the server sends this message to send audio data to the peer.
     */
            AUDIO = 0x08,
    /**
     * RTMP video packet (0x09)
     * The client or the server sends this message to send video data to the peer.
     */
            VIDEO = 0x09,
    /**
     * RTMP message type 0x0F
     * The client or the server sends this message to send Metadata or any
     * user data to the peer. Metadata includes details about the data (audio, video etc.)
     * like creation time, duration, theme and so on.
     * This is the AMF3-encoded version.
     */
            DATA_AMF3 = 0x0F,
    /**
     * RTMP message type 0x10
     * A shared object is a Flash object (a collection of name value pairs)
     * that are in synchronization across multiple clients, instances, and
     * so on.
     * This is the AMF3 version: kMsgContainerEx=16 for AMF3.
     */
            SHARED_OBJECT_AMF3 = 0x10,
    /**
     * RTMP message type 0x11
     * Command messages carry the AMF-encoded commands between the client
     * and the server.
     * A command message consists of command name, transaction ID, and command object that
     * contains related parameters.
     * This is the AMF3-encoded version.
     */
            COMMAND_AMF3 = 0x11,
    /**
     * RTMP message type 0x12
     * The client or the server sends this message to send Metadata or any
     * user data to the peer. Metadata includes details about the data (audio, video etc.)
     * like creation time, duration, theme and so on.
     * This is the AMF0-encoded version.
     */
            DATA_AMF0 = 0x12,
    /**
     * RTMP message type 0x14
     * Command messages carry the AMF-encoded commands between the client
     * and the server.
     * A command message consists of command name, transaction ID, and command object that
     * contains related parameters.
     * This is the common AMF0 version, also known as INVOKE in some RTMP implementations.
     */
            COMMAND_AMF0 = 0x14,
    /**
     * RTMP message type 0x13
     * A shared object is a Flash object (a collection of name value pairs)
     * that are in synchronization across multiple clients, instances, and
     * so on.
     * This is the AMF0 version: kMsgContainer=19 for AMF0.
     */
            SHARED_OBJECT_AMF0 = 0x13,
    /**
     * RTMP message type 0x16
     * An aggregate message is a single message that contains a list of sub-messages.
     */
            AGGREGATE_MESSAGE = 0x16


};

enum ChunkType {

    /** Full 12-byte RTMP chunk header */
            TYPE_0_FULL = 0x00,
    /** Relative 8-byte RTMP chunk header (message stream ID is not included) */
            TYPE_1_RELATIVE_LARGE = 0x01,
    /** Relative 4-byte RTMP chunk header (only timestamp delta) */
            TYPE_2_RELATIVE_TIMESTAMP_ONLY = 0x02,
    /** Relative 1-byte RTMP chunk header (no "real" header, just the 1-byte indicating chunk header type & chunk stream ID) */
            TYPE_3_RELATIVE_SINGLE_BYTE = 0x03
    /** The byte value of this chunk header type */

};


enum Type {

    /**
     * Type: 0
     * The server sends this event to notify the client that a stream has become
     * functional and can be used for communication. By default, this event
     * is sent on ID 0 after the application connect command is successfully
     * received from the client.
     *
     * Event Data:
     * eventData[0] (int) the stream ID of the stream that became functional
     */
            STREAM_BEGIN = 0,
    /**
     * Type: 1
     * The server sends this event to notify the client that the playback of
     * data is over as requested on this stream. No more data is sent without
     * issuing additional commands. The client discards the messages received
     * for the stream.
     *
     * Event Data:
     * eventData[0]: the ID of thestream on which playback has ended.
     */
            STREAM_EOF = 1,
    /**
     * Type: 2
     * The server sends this event to notify the client that there is no
     * more data on the stream. If the server does not detect any message for
     * a time period, it can notify the subscribed clients that the stream is
     * dry.
     *
     * Event Data:
     * eventData[0]: the stream ID of the dry stream.
     */
            STREAM_DRY = 2,
    /**
     * Type: 3
     * The client sends this event to inform the server of the buffer size
     * (in milliseconds) that is used to buffer any data coming over a stream.
     * This event is sent before the server starts  processing the stream.
     *
     * Event Data:
     * eventData[0]: the stream ID and
     * eventData[1]: the buffer length, in milliseconds.
     */
            SET_BUFFER_LENGTH = 3,
    /**
     * Type: 4
     * The server sends this event to notify the client that the stream is a
     * recorded stream.
     *
     * Event Data:
     * eventData[0]: the stream ID of the recorded stream.
     */
            STREAM_IS_RECORDED = 4,
    /**
     * Type: 6
     * The server sends this event to test whether the client is reachable.
     *
     * Event Data:
     * eventData[0]: a timestamp representing the local server time when the server dispatched the command.
     *
     * The client responds with PING_RESPONSE on receiving PING_REQUEST.
     */
            PING_REQUEST = 6,
    /**
     * Type: 7
     * The client sends this event to the server in response to the ping request.
     *
     * Event Data:
     * eventData[0]: the 4-byte timestamp which was received with the PING_REQUEST.
     */
            PONG_REPLY = 7,
    /**
     * Type: 31 (0x1F)
     *
     * This user control type is not specified in any official documentation, but
     * is sent by Flash Media Server 3.5. Thanks to the rtmpdump devs for their
     * explanation:
     *
     * Buffer Empty (unofficial name): After the server has sent a complete buffer, and
     * sends this Buffer Empty message, it will wait until the play
     * duration of that buffer has passed before sending a new buffer.
     * The Buffer Ready message will be sent when the new buffer starts.
     *
     * (see also: http://repo.or.cz/w/rtmpdump.git/blob/8880d1456b282ee79979adbe7b6a6eb8ad371081:/librtmp/rtmp.c#l2787)
     */
            BUFFER_EMPTY = 31,
    /**
     * Type: 32 (0x20)
     *
     * This user control type is not specified in any official documentation, but
     * is sent by Flash Media Server 3.5. Thanks to the rtmpdump devs for their
     * explanation:
     *
     * Buffer Ready (unofficial name): After the server has sent a complete buffer, and
     * sends a Buffer Empty message, it will wait until the play
     * duration of that buffer has passed before sending a new buffer.
     * The Buffer Ready message will be sent when the new buffer starts.
     * (There is no BufferReady message for the very first buffer;
     * presumably the Stream Begin message is sufficient for that
     * purpose.)
     *
     * (see also: http://repo.or.cz/w/rtmpdump.git/blob/8880d1456b282ee79979adbe7b6a6eb8ad371081:/librtmp/rtmp.c#l2787)
     */
            BUFFER_READY = 32

};

enum LimitType {

    /**
     * In a hard (0) request, the peer must send the data in the provided bandwidth.
     */
            HARD = 0,
    /**
     * In a soft (1) request, the bandwidth is at the discretion of the peer
     * and the sender can limit the bandwidth.
     */
            SOFT = 1,
    /**
     * In a dynamic (2) request, the bandwidth can be hard or soft.
     */
            DYNAMIC = 2

};


class RtmpPacketHeader {
private :
    ChunkType chunkType;
    //这个需要用2bit来表示 00:0 01:1 10:2 11:3  转换(二进制:十进制)
    int chunkStreamId;
    //根据ch
    int absoluteTimestamp = 0;
    int timestampDelta = -1;
    int packetLength;
    MessageType messageType;
    int messageStreamId = 0;
    int extendedTimestamp = 0;


public:
    RtmpPacketHeader();

    RtmpPacketHeader(MessageType messageType, ChunkType chunkType, int chunkStreamId);


    /** @return the RTMP chunk stream ID (channel ID) for this chunk */
    int getChunkStreamId();

    ChunkType getChunkType();

    int getPacketLength();

    int getMessageStreamId();

    MessageType getMessageType();

    int getAbsoluteTimestamp();

    void setAbsoluteTimestamp(int absoluteTimestamp);

    int getTimestampDelta();

    void setTimestampDelta(int timestampDelta);

    /** Sets the RTMP chunk stream ID (channel ID) for this chunk */
    void setChunkStreamId(int channelId);

    void setChunkType(ChunkType chunkType);

    void setMessageStreamId(int messageStreamId);

    void setMessageType(MessageType messageType);

    void setPacketLength(int packetLength);


    int getExtendedTimestamp() const;

    void setExtendedTimestamp(int extendedTimestamp);
};


class RtmpPacket {
private:
    RtmpPacketHeader header;

public:

    RtmpPacket(RtmpPacketHeader header);


    RtmpPacketHeader &getHeader();

public:
    virtual int readBody(RtmpStream *stream) = 0;

    virtual int writeBody(RtmpStream *stream) = 0;

    virtual int getSize() = 0;

    virtual ~RtmpPacket();
};

class SetChunkSize : public RtmpPacket {
private:
    int chunkSize;

public:
    SetChunkSize(const RtmpPacketHeader &header);

    SetChunkSize();

    int readBody(RtmpStream *stream);

    int getChunkSize() {
        return chunkSize;
    }

    int writeBody(RtmpStream *socket);

    int getSize();

};

class Abort : public RtmpPacket {
private:
    int chunkStreamId;

public:
    Abort(const RtmpPacketHeader &header);

    Abort(int chunkStreamId);

    const int &getChunkStreamId() const;

    int readBody(RtmpStream *stream);

    int writeBody(RtmpStream *stream);

    int getSize();
};

class UserControl : public RtmpPacket {

private:
    Type type;
    int *eventData = NULL;
public:
    UserControl(const RtmpPacketHeader &header);

    UserControl(ChunkType chunkType);

    UserControl(Type type, ChunkType chunkType);

    UserControl(UserControl &control, ChunkType chunkType);

    /** Used to set (a single) event data for most user control message types */
    int setEventData(int eventData);

    /** Used to set event data for the SET_BUFFER_LENGTH user control message types */
    int setEventData(int streamId, int bufferLength);

    int readBody(RtmpStream *stream);

    int writeBody(RtmpStream *stream);

    ~UserControl();

    int getSize();

    const Type &getType() const;
};


class WindowAckSize : public RtmpPacket {
private :
    int acknowledgementWindowSize;

public:
    WindowAckSize(const RtmpPacketHeader &header);

    WindowAckSize(int acknowledgementWindowSize, ChunkType type);


    int getAcknowledgementWindowSize() const;

    void setAcknowledgementWindowSize(int acknowledgementWindowSize);

    int readBody(RtmpStream *stream);

    int writeBody(RtmpStream *stream);

    int getSize();

};


class SetPeerBandwidth : public RtmpPacket {
private :
    int acknowledgementWindowSize;
    LimitType limitType;
public:
    SetPeerBandwidth(const RtmpPacketHeader &header);

    SetPeerBandwidth(int acknowledgementWindowSize, ChunkType type, LimitType l_type);

    int getAcknowledgementWindowSize() const;

    void setAcknowledgementWindowSize(int acknowledgementWindowSize);

    LimitType getLimitType() const;

    void setLimitType(LimitType limitType);

    int readBody(RtmpStream *stream);

    int writeBody(RtmpStream *stream);

    int getSize();

};

class ContentData : public RtmpPacket {
protected:
    byte *data;
    int size;

public:
    ContentData(const RtmpPacketHeader &header);

    void setData(byte *data, int size);

    byte *getData();

    int readBody(RtmpStream *stream);

    int writeBody(RtmpStream *stream);

    virtual~ ContentData();

    int getSize();

};

class Audio : public ContentData {
public:
    Audio(const RtmpPacketHeader &header);

    Audio();
};

class Video : public ContentData {

public:
    Video(const RtmpPacketHeader &header);

    Video() ;
};


class VariableBodyRtmpPacket : public RtmpPacket {
protected:
    std::vector<AmfData *> *items = NULL;

public:

    VariableBodyRtmpPacket(const RtmpPacketHeader &header);

    std::vector<AmfData *> *getItems();

    void addStringData(std::string str);

    void addDoubleData(double number);

    void addBoolData(bool flag);

    void addAmfData(AmfData *dataItem);

    ~VariableBodyRtmpPacket();

    int getSize();

protected :
    void readVariableData(RtmpStream *in, int bytesAlreadyRead);

    void writeVariableData(RtmpStream *in);


};

class Command : public VariableBodyRtmpPacket {
private:
    int transactionId;
    std::string commandName;
public:
    Command(const RtmpPacketHeader &header);

    Command(std::string commandName, int transactionId, ChunkType type);

    Command(std::string commandName, int transactionId);

    const std::string &getCommandName() const;

    int getTransactionId() const;

    int readBody(RtmpStream *stream);

    int writeBody(RtmpStream *stream);

    int getSize();


};

class Data : public VariableBodyRtmpPacket {
private :
    std::string type;
public:

    Data(const RtmpPacketHeader &header);


    Data(std::string type);


    const std::string &getType();


    void setType(std::string type);

    int readBody(RtmpStream *in);

    int writeBody(RtmpStream *stream);

    int getSize();
};


class Acknowledgement : public RtmpPacket {
private:
    int sequenceNumber;
public:
    Acknowledgement(const RtmpPacketHeader &header);

    Acknowledgement(int numBytesReadThusFar);

    /** @return the sequence number, which is the number of the bytes received so far */
    int getSequenceNumber();

    /** Sets the sequence number, which is the number of the bytes received so far */
    void setSequenceNumber(int numBytesRead);

    int readBody(RtmpStream *in);

    int writeBody(RtmpStream *out);

    int getSize();
};

#endif //SOCKET_RTMP_PACKET_H

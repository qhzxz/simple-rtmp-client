//
// Created by 秦汉 on 16/7/12.



#ifndef SOCKET_RTMP_IO_H
#define SOCKET_RTMP_IO_H

#include <map>
#include <string>
#include <mutex>
#include <sys/time.h>
#include <vector>

#include "rtmp_packet.h"

class ChunkStreamInfo {

    //这里的Channel 其实就是 stream的Id

private:
    RtmpPacketHeader *prevHeaderRx = NULL;//上一个读取Chunk的 Message Header 用于后面的 Header 数据读取

    RtmpPacketHeader *prevHeaderTx = NULL;//上一个传输Chunk的 Message Header 用于后面的 Header 数据读取

    static long sessionBeginTimestamp;

    long realLastTimestamp = 0;  // Do not use wall time!

    std::vector<byte> baos;

    /** @return the previous header that was received on this channel, or <code>null</code> if no previous header was received */
public:

    ChunkStreamInfo();


    RtmpPacketHeader *getPrevHeaderRx();

    /** Sets the previous header that was received on this channel, or <code>null</code> if no previous header was sent */


    void setPrevHeaderRx(RtmpPacketHeader *previousHeader);

    /** @return the previous header that was transmitted on this channel */


    RtmpPacketHeader *getPrevHeaderTx();

    /**
     * 这是在比较 前后 两个chunk message header 中的 message type 是否一样 如果一样 那么后面一个
     * chunk message header 中的某些数据就可以参照前一个 chunk message header
     *
     * @param forMessageType
     * @return
     */


    bool canReusePrevHeaderTx(MessageType forMessageType);

    /** Sets the previous header that was transmitted on this channel */


    void setPrevHeaderTx(RtmpPacketHeader *prevHeaderTx);

    /** Sets the session beginning timestamp for all chunks */


    static void markSessionTimestampTx();

    /** Utility method for calculating & synchronizing transmitted timestamps */


    long markAbsoluteTimestampTx();

    /** Utility method for calculating & synchronizing transmitted timestamp deltas */


    long markDeltaTimestampTx();

    /** @return <code>true</code> if all packet data has been stored, or <code>false</code> if not */

    int storePacketChunk(RtmpSocket *socket, int chunkSize);

    RtmpByteArrayStream *getStoredPacketData();

    /** Clears all currently-stored packet chunks (used when an ABORT packet is received) */

    void clearStoredChunks();

    ~ChunkStreamInfo();

};

class RtmpSessionInfo {
    /** The (total) number of bytes read for this window (resets to 0 if the agreed-upon RTMP window acknowledgement size is reached) */
private :
    int windowBytesRead;
    /** The window acknowledgement size for this RTMP session, in bytes; default to max to avoid unnecessary "Acknowledgment" messages from being sent */
    int acknowledgementWindowSize = 65535;
    /** Used internally to store the total number of bytes read (used when sending Acknowledgement messages) */
    int totalBytesRead = 0;

    /** Default chunk size is 128 bytes */
    int rxChunkSize = 128;
    int txChunkSize = 128;
    std::mutex methodLock;
    typedef unsigned int KeyType;
    typedef std::map<KeyType, ChunkStreamInfo *> ChunkChannelsMap;
    typedef std::pair<KeyType, ChunkStreamInfo *> ChunkPair;
    typedef std::map<KeyType, std::string> MethodMap;
    typedef std::pair<KeyType, std::string> MethodPair;

    ChunkChannelsMap chunkChannels;
    MethodMap invokedMethods;


public :
    ChunkStreamInfo *getChunkStreamInfo(int chunkStreamId);

    std::string takeInvokedCommand(int transactionId);

    std::string addInvokedCommand(int transactionId, std::string commandName);

    int getRxChunkSize();

    void setRxChunkSize(int chunkSize);

    int getTxChunkSize();

    void setTxChunkSize(int chunkSize);

    int getAcknowledgementWindowSize();

    void setAcknowledgmentWindowSize(int acknowledgementWindowSize);

    /**
     * Add the specified amount of bytes to the total number of bytes read for this RTMP window;
     * @param numBytes the number of bytes to add
     * @return <code>true</code> if an "acknowledgement" packet should be sent, <code>false</code> otherwise
     */
    bool addToWindowBytesRead(const int numBytes);

    ~RtmpSessionInfo();
};


#endif //SOCKET_RTMP_IO_H

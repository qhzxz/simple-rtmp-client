//
// Created by 秦汉 on 16/7/29.
//


#include <iostream>
#include "rtmp_connection.h"
#include "rtmp_complex_hand_shake_v3.h"
#include "rtmp_log.h"


RtmpConnection::RtmpConnection() {
}

int RtmpConnection::handShake() {
    int ret = RESULT_SUCCESS;
    if (NULL == rtmpSocket) {
        return RESULT_NULL_POINTER;
    }
    RtmpHandShake rtmpHandShake;
    ret = rtmpHandShake.handShake(*rtmpSocket);
    return ret;
}

int RtmpConnection::connect(const std::string &url) {
    int ret = RESULT_SUCCESS;
    std::regex urlPattern("^rtmp://([^/:]+)(:(\\d+))*/([^/]+)(/(.*))*$");
    std::match_results<std::string::const_iterator> results;
    if ((ret = checkUrlPattern(urlPattern, results, url)) != RESULT_SUCCESS) {
        return ret;
    }
    tcUrl = url.substr(0, url.find_last_of('/'));
    swfUrl = "";
    pageUrl = "";
    std::string host = results[1];
    std::string port_str = results[3];
    int port = port_str.empty() ? 1935 : atoi(port_str.data());
    appName = results[4];
    streamName = results[6];
    rtmpSocket = new RtmpSocket(host, port);
    if ((ret = rtmpSocket->connect()) != RESULT_SUCCESS) {
        return ret;
    }
    if ((ret = handShake()) != RESULT_SUCCESS) {
        return ret;
    }
    active = true;
    sessionInfo = new RtmpSessionInfo();
    rtmpReadThread = new RtmpReadThread(false, sessionInfo, rtmpSocket);
    rtmpWriteThread = new RtmpWriteThread(false, sessionInfo, rtmpSocket);
    rtmpReadThread->setHandler(this);
    rtmpReadThread->start();
    rtmpWriteThread->start();
    std::thread handle_thread(&RtmpConnection::handleRxPacketLoop, this);
    handle_thread.detach();
    ret = rtmpConnect();
    return ret;
}


int RtmpConnection::rtmpConnect() {
    int ret = RESULT_SUCCESS;
    if (fullyConnected || connecting) {
        return RESULT_ALREADY_CONNECTED_OR_CONNECTING;
    }

    // Mark session timestamp of all chunk stream information on connection.
    ChunkStreamInfo::markSessionTimestampTx();

//    Log.d(TAG, "rtmpConnect(): Building 'connect' invoke packet");
    ChunkStreamInfo *chunkStreamInfo = sessionInfo->getChunkStreamInfo(RTMP_COMMAND_CHANNEL);
    bool canReuse = chunkStreamInfo->canReusePrevHeaderTx(COMMAND_AMF0);
    Command *invoke = new Command("connect", ++transactionIdCounter, canReuse ? TYPE_1_RELATIVE_LARGE : TYPE_0_FULL);
    invoke->getHeader().setMessageStreamId(0);
    AmfObject *args = new AmfObject();
    args->setStringProperty("app", appName);
    args->setStringProperty("flashVer", "LNX 11,2,202,233"); // Flash player OS: Linux, version: 11.2.202.233
    args->setStringProperty("swfUrl", swfUrl);
    args->setStringProperty("tcUrl", tcUrl);
    args->setBoolProperty("fpad", false);
    args->setIntProperty("capabilities", 239);
    args->setIntProperty("audioCodecs", 3575);
    args->setIntProperty("videoCodecs", 252);
    args->setIntProperty("videoFunction", 1);
    args->setStringProperty("pageUrl", "");
    args->setIntProperty("objectEncoding", 0);
    invoke->addAmfData(args);
    rtmpWriteThread->send(invoke);
    connecting = true;
    return ret;
}

int RtmpConnection::checkUrlPattern(std::regex &parrten, std::match_results<std::string::const_iterator> &results,
                                    const std::string &url) {
    int ret = RESULT_SUCCESS;
    if (!std::regex_match(url, results, parrten)) {
        ret = RESULT_URL_PATTERN_NOT_MATCH;
    }
    return ret;
}

RtmpConnection::~RtmpConnection() {
    if (NULL != sessionInfo) {
        delete sessionInfo;
        sessionInfo = NULL;
    }
    if (NULL != rtmpWriteThread) {
        delete rtmpWriteThread;
        rtmpWriteThread = NULL;
    }
    if (NULL != rtmpReadThread) {
        delete rtmpReadThread;
        rtmpReadThread = NULL;
    }

    if (NULL != rtmpSocket) {
        delete rtmpSocket;
        rtmpSocket = NULL;
    }
    clearPacket();
}

void RtmpConnection::handleRxPacket(RtmpPacket *packet) {
    std::unique_lock<std::mutex> lck(dequeLock);
    rxPacketDeque.push_back(packet);
}

void RtmpConnection::handleRxPacketLoop() {

    while (active) {
        while (!rxPacketDeque.empty()) {
            dequeLock.lock();
            RtmpPacket *packet = rxPacketDeque.at(0);
            rxPacketDeque.pop_front();
            dequeLock.unlock();
            int ret = RESULT_SUCCESS;
            switch (packet->getHeader().getMessageType()) {
                case ABORT: {
                    Abort *abort = dynamic_cast<Abort *>(packet);
                    if (NULL != abort) {
                        sessionInfo->getChunkStreamInfo(abort->getChunkStreamId())->clearStoredChunks();
                    }
                }
                    break;
                case USER_CONTROL_MESSAGE: {
                    UserControl *ping = dynamic_cast<UserControl *>(packet);
                    if (NULL != ping) {
                        switch (ping->getType()) {
                            case PING_REQUEST: {
                                ChunkStreamInfo *channelInfo = sessionInfo->getChunkStreamInfo(
                                        RTMP_CONTROL_CHANNEL);
                                RTMP_LOG_INFO("handleRxPacketLoop(): Sending PONG reply..");
                                bool canResue = channelInfo->canReusePrevHeaderTx(USER_CONTROL_MESSAGE);
                                UserControl *pong = new UserControl(*ping,
                                                                    canResue ? TYPE_2_RELATIVE_TIMESTAMP_ONLY
                                                                             : TYPE_0_FULL);
                                rtmpWriteThread->send(pong);
                            }
                                break;
                            case STREAM_EOF:
                                RTMP_LOG_INFO("handleRxPacketLoop(): Stream EOF reached, closing RTMP writer...");
                                break;
                        }

                    }
                }
                    break;
                case WINDOW_ACKNOWLEDGEMENT_SIZE: {
                    WindowAckSize *windowAckSize = dynamic_cast<WindowAckSize *>(packet);
                    int size = windowAckSize->getAcknowledgementWindowSize();
                    RTMP_LOG_INFO("handleRxPacketLoop(): Setting acknowledgement window size: {0:d}",
                                  size);
                    sessionInfo->setAcknowledgmentWindowSize(size);
                    // Set socket option
                    ret = rtmpSocket->setSocketSendBufferSize(size);
                    if (ret != RESULT_SUCCESS) {
                        RTMP_LOG_INFO("handle read error,ret:{0:d}", ret);
                        shutdown();
                    }
                }
                    break;
                case SET_PEER_BANDWIDTH: {
                    int acknowledgementWindowsize = sessionInfo->getAcknowledgementWindowSize();
                    ChunkStreamInfo *chunkStreamInfo = sessionInfo->getChunkStreamInfo(RTMP_CONTROL_CHANNEL);
                    RTMP_LOG_INFO("handleRxPacketLoop(): Send acknowledgement window size: {0:d}",
                                  acknowledgementWindowsize);
                    bool canReuseChuckHeader = chunkStreamInfo->canReusePrevHeaderTx(WINDOW_ACKNOWLEDGEMENT_SIZE);
                    WindowAckSize *ackSize = new WindowAckSize(acknowledgementWindowsize,
                                                               canReuseChuckHeader ? TYPE_2_RELATIVE_TIMESTAMP_ONLY
                                                                                   : TYPE_0_FULL);
                    rtmpWriteThread->send(ackSize);
                }
                    break;
                case COMMAND_AMF0:
                    ret = handleRxInvoke(packet);
                    if (ret != RESULT_SUCCESS) {
                        RTMP_LOG_INFO("handle read error,ret:{0:d}", ret);
                        shutdown();
                    }
                    break;
                default:
                    RTMP_LOG_INFO("handleRxPacketLoop(): Not handling unimplemented/unknown packet of type: {0:x}",
                                  packet->getHeader().getMessageType());
                    break;
            }
            delete packet;
            packet = NULL;
        }
    }
}

int RtmpConnection::handleRxInvoke(RtmpPacket *packet) {
    int ret = RESULT_SUCCESS;
    Command *invoke = dynamic_cast<Command *>(packet);
    if (NULL != invoke) {
        std::string commandName = invoke->getCommandName();
        RTMP_LOG_INFO("commandName=" + commandName);
        if ("_result" == commandName) {
            // This is the result of one of the methods invoked by us
            std::string method = sessionInfo->takeInvokedCommand(invoke->getTransactionId());
            RTMP_LOG_INFO("handleRxInvoke: Got result for invoked method: {0}", method);
            if ("connect" == method) {
                // Capture server ip/pid/id information if any
//                String serverInfo = onSrsServerInfo(invoke);
//                mHandler.onRtmpConnected("connected" + serverInfo);
                // We can now send createStream commands
                connecting = false;
                fullyConnected = true;
                std::unique_lock<std::mutex> lck(connectLock);
                connect_cv.notify_all();
            } else if ("createStream" == method) {
                // Get stream id
                std::vector<AmfData *> *pVector = invoke->getItems();
                AmfNumber *number = dynamic_cast<AmfNumber *>(pVector->at(1));
                currentStreamId = (int) (number->getValue());
                RTMP_LOG_INFO("handleRxInvoke(): Stream ID to publish: {0:d}", currentStreamId);
                if (!streamName.empty() && !publishType.empty()) {
                    ret = fmlePublish();
                }
            } else if ("releaseStream" == method) {
                RTMP_LOG_INFO("handleRxInvoke(): 'releaseStream'");
            } else if ("FCPublish" == method) {
                RTMP_LOG_INFO("handleRxInvoke(): 'FCPublish'");
            } else {
                RTMP_LOG_INFO("handleRxInvoke(): '_result' message received for unknown method: {0}", method);
            }
        } else if ("onBWDone" == commandName) {
            RTMP_LOG_INFO("handleRxInvoke(): 'onBWDone'");
        } else if ("onFCPublish" == commandName) {
            RTMP_LOG_INFO("handleRxInvoke(): 'onFCPublish'");
        } else if ("onStatus" == commandName) {
            std::string code;
            std::vector<AmfData *> *pVector = invoke->getItems();
            AmfObject *object = dynamic_cast<AmfObject *>(pVector->at(1));
            if (NULL != object) {
                AmfString *string = dynamic_cast<AmfString *>(object->getProperty("code"));
                if (NULL != string) {
                    code = string->getValue();
                }
            }
            RTMP_LOG_INFO("onStatus code: {0}", code);
            if ("NetStream.Publish.Start" == code) {
                ret = onMetaData();
                // We can now publish AV data
                publishPermitted = true;
                std::unique_lock<std::mutex> lck(publishLock);
                publish_cv.notify_all();

            } else if ("NetStream.Publish.BadName" == code) {
                std::unique_lock<std::mutex> lck(publishLock);
                publish_cv.notify_all();
            }
        } else {
            RTMP_LOG_INFO("handleRxInvoke(): Unknown/unhandled server invoke ");
        }

    }
    return ret;

}

int RtmpConnection::onMetaData() {
    int ret = RESULT_SUCCESS;
    if (!fullyConnected) {
        return RESULT_NOT_CONNECTED_TO_SERVER;
    }

    if (currentStreamId == -1) {
        return RESULT_NO_CURRENT_STREAM_OBJECT_EXIST;
    }

    Data *meta = new Data("@setDataFrame");
    meta->getHeader().setMessageStreamId(currentStreamId);
    meta->addStringData(std::string("onMetaData"));
    AmfEcmaArray *ecmaArray = new AmfEcmaArray();
    ecmaArray->setIntProperty("duration", 0);
    ecmaArray->setIntProperty("width", 400);
    ecmaArray->setIntProperty("height", 400);
    ecmaArray->setIntProperty("videodatarate", 0);
    ecmaArray->setIntProperty("framerate", 0);
    ecmaArray->setIntProperty("audiodatarate", 0);
    ecmaArray->setIntProperty("audiosamplerate", 44100);
    ecmaArray->setIntProperty("audiosamplesize", 16);
    ecmaArray->setBoolProperty("stereo", true);
    ecmaArray->setIntProperty("filesize", 0);
    meta->addAmfData(ecmaArray);
    rtmpWriteThread->send(meta);
    return ret;
}


int RtmpConnection::fmlePublish() {
    int ret = RESULT_SUCCESS;
    if (!fullyConnected) {
        return RESULT_NOT_CONNECTED_TO_SERVER;
    }

    if (currentStreamId == -1) {
        return RESULT_NO_CURRENT_STREAM_OBJECT_EXIST;
    }
    Command *publish = new Command("publish", 0);
    publish->getHeader().setChunkStreamId(RTMP_STREAM_CHANNEL);
    publish->getHeader().setMessageStreamId(currentStreamId);
    publish->addAmfData(new AmfNull());  // command object: null for "publish"
    publish->addStringData(streamName);
    publish->addStringData(publishType);
    rtmpWriteThread->send(publish);
    return ret;
}

int RtmpConnection::createStream() {
    int ret = RESULT_SUCCESS;
    if (!fullyConnected) {
        return RESULT_NOT_CONNECTED_TO_SERVER;
    }

    if (currentStreamId != -1) {
        return RESULT_CURRENT_STREAM_OBJECT_EXIST;
    }
    Command *releaseStream = new Command("releaseStream", ++transactionIdCounter);
    releaseStream->getHeader().setChunkStreamId(RTMP_STREAM_CHANNEL);
    releaseStream->addAmfData(new AmfNull());  // command object: null for "createStream"
    releaseStream->addStringData(streamName);  // command object: null for "releaseStream"
    rtmpWriteThread->send(releaseStream);

//    Log.d(TAG, "createStream(): Sending FCPublish command...");
    // transactionId == 3
    Command *FCPublish = new Command("FCPublish", ++transactionIdCounter);
    FCPublish->getHeader().setChunkStreamId(RTMP_STREAM_CHANNEL);
    FCPublish->addAmfData(new AmfNull());  // command object: null for "FCPublish"
    FCPublish->addStringData(streamName);
    rtmpWriteThread->send(FCPublish);

//    Log.d(TAG, "createStream(): Sending createStream command...");
    ChunkStreamInfo *chunkStreamInfo = sessionInfo->getChunkStreamInfo(RTMP_COMMAND_CHANNEL);
    // transactionId == 4
    bool can_resue_chuck_header = chunkStreamInfo->canReusePrevHeaderTx(COMMAND_AMF0);
    Command *createStream = new Command("createStream", ++transactionIdCounter,
                                        can_resue_chuck_header ? TYPE_1_RELATIVE_LARGE : TYPE_0_FULL);
    createStream->addAmfData(new AmfNull());  // command object: null for "createStream"
    rtmpWriteThread->send(createStream);
    std::unique_lock<std::mutex> lck(publishLock);
    publish_cv.wait_for(lck, std::chrono::milliseconds(500));
    return ret;
}

int RtmpConnection::closeStream() {
    int ret = RESULT_SUCCESS;
    if (!fullyConnected) {
        return RESULT_NOT_CONNECTED_TO_SERVER;
    }

    if (currentStreamId == -1) {
        return RESULT_NO_CURRENT_STREAM_OBJECT_EXIST;
    }
    if (!publishPermitted) {
        return RESULT_NOT_GET_START;
    }

    Command *closeStream = new Command("closeStream", 0);
    closeStream->getHeader().setChunkStreamId(RTMP_STREAM_CHANNEL);
    closeStream->getHeader().setMessageStreamId(currentStreamId);
    closeStream->addAmfData(new AmfNull());
    rtmpWriteThread->send(closeStream);

    return ret;
}

int RtmpConnection::publish(std::string type) {
    if (connecting) {
        std::unique_lock<std::mutex> lck(connectLock);
        connect_cv.wait_for(lck, std::chrono::milliseconds(5000));
    }
    publishType = type;
    int ret = createStream();
    return ret;
}

void RtmpConnection::clearPacket() {
    std::unique_lock<std::mutex> lck(dequeLock);
    for (auto dequeIterator = rxPacketDeque.begin(); dequeIterator != rxPacketDeque.end(); ++dequeIterator) {
        RtmpPacket *packet = *dequeIterator;
        delete packet;
        packet = NULL;
    }
    rxPacketDeque.clear();
}

void RtmpConnection::reset() {
    active = false;
    connecting = false;
    fullyConnected = false;
    publishPermitted = false;
    tcUrl = "";
    swfUrl = "";
    pageUrl = "";
    appName = "";
    streamName = "";
    publishType = "";
    currentStreamId = -1;
    transactionIdCounter = 0;
    delete sessionInfo;
    sessionInfo = NULL;
    clearPacket();
}

void RtmpConnection::shutdown() {
    if (NULL != rtmpSocket) {
        rtmpSocket->closeSocket();
    }
    if (NULL != rtmpReadThread) {
        rtmpReadThread->stop();
    }
    if (NULL != rtmpWriteThread) {
        rtmpWriteThread->stop();
    }

    active = false;

    delete rtmpWriteThread;
    rtmpWriteThread = NULL;

    delete rtmpReadThread;
    rtmpReadThread = NULL;

    delete rtmpSocket;
    rtmpSocket = NULL;

    reset();
}

void RtmpConnection::notifyWindowAckRequired(const int numBytesReadThusFar) {
    rtmpWriteThread->send(new Acknowledgement(numBytesReadThusFar));
}

void RtmpConnection::publishAudioData(int size, int32_t time_stamp, byte *data) {
    Audio *audio = new Audio;
    audio->getHeader().setAbsoluteTimestamp(time_stamp);
    audio->getHeader().setMessageStreamId(currentStreamId);
    audio->setData(data, size);
    rtmpWriteThread->send(audio);
}

void RtmpConnection::publishVideoData(int size, int32_t time_stamp, byte *data) {
    Video *video = new Video;
    video->getHeader().setAbsoluteTimestamp(time_stamp);
    video->getHeader().setMessageStreamId(currentStreamId);
    video->setData(data, size);
    rtmpWriteThread->send(video);
}
//
// Created by 秦汉 on 16/7/14.
//

#ifndef SOCKET_RTMP_TYPE_H
#define SOCKET_RTMP_TYPE_H


typedef unsigned char byte;


static const byte RTMP_STREAM_CHANNEL = 0x05;
static const byte RTMP_COMMAND_CHANNEL = 0x03;
static const byte RTMP_VIDEO_CHANNEL = 0x06;
static const byte RTMP_AUDIO_CHANNEL = 0x07;
static const byte RTMP_CONTROL_CHANNEL = 0x02;


static const int RESULT_SUCCESS = 1;
static const int RESULT_UNKNOWN_CHUCK_HEADER_TYPE = 1000;
static const int RESULT_FAIL_SET_USER_CONTROL_EVENT_DATA = 1001;
static const int RESULT_UNKNOWN_MESSAGE_TYPE = 1002;
static const int RESULT_PACKET_NOT_YET_COMPLETE = 1003;

static const int RESULT_SOCKET_TIME_OUT = 2001;
static const int RESULT_SOCKET_END = 2002;
static const int RESULT_SET_SOCKET_WRITE_TIME_FAIL = 2003;
static const int RESULT_SET_SOCKET_READ_TIME_FAIL = 2004;
static const int RESULT_SET_SOCKET_SEND_BUFFER_SIZE_FAIL = 2005;
static const int RESULT_SOCKET_CLOSE_FAIL = 2006;
static const int RESULT_NOT_CONNECT = 2007;


static const int RESULT_HAND_SHAKE_NOT_VALIDATE = 3000;
static const int RESULT_NOT_CONNECTED_TO_SERVER = 3001;
static const int RESULT_NO_CURRENT_STREAM_OBJECT_EXIST = 3002;
static const int RESULT_NOT_GET_START = 3003;
static const int RESULT_ALREADY_CONNECTED_OR_CONNECTING = 3004;
static const int RESULT_CURRENT_STREAM_OBJECT_EXIST = 3005;

static const int RESULT_NULL_POINTER = 4000;
static const int RESULT_URL_PATTERN_NOT_MATCH = 4001;

static const int RESULT_NO_PREV_HEADER = 5000;
#endif //SOCKET_RTMP_TYPE_H

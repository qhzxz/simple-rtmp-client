#include <iostream>
#include "rtmp_connection.h"
#include "rtmp_log.h"


using namespace std;


int main() {

    RtmpConnection connection;
    int ret = connection.connect("rtmp://172.17.17.64/SOSample/stream");
    if (ret!=RESULT_SUCCESS) {
        RTMP_LOG_INFO("connect fail");
    }
    ret = connection.publish("live");
    if (ret!=RESULT_SUCCESS) {
        RTMP_LOG_INFO("publish fail");
    }
    while (true) {}
    return 0;
}
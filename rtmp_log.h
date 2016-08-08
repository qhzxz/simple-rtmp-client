//
// Created by 秦汉 on 16/8/4.
//

#ifndef SOCKET_RTMP_LOG_H
#define SOCKET_RTMP_LOG_H


#include <string>
#include "spdlog.h"

template<typename  ...Args>
inline void RTMP_LOG_INFO(std::string info, const Args&...args) {
    auto console = spdlog::get("console");
    if (NULL == console) {
        console = spdlog::stdout_logger_mt("console", true);
    }
    console->info(info.c_str(), args...);
};
#endif //SOCKET_RTMP_LOG_H

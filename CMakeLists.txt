cmake_minimum_required(VERSION 3.5)
project(simple_rtmp_client CXX)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include/spdlog)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include/openssl)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp rtmp_packet.cpp rtmp_packet.h rtmp_io.h rtmp_io.cpp rtmp_socket.cpp rtmp_socket.h rtmp_utility.h rtmp_utility.cpp rtmp_type.h rtmp_packet_reader.cpp rtmp_packet_reader.h rtmp_stream.cpp rtmp_stream.h rtmp_amf0.cpp rtmp_amf0.h rtmp_packet_writer.cpp rtmp_packet_writer.h rtmp_read_thread.cpp rtmp_read_thread.h rtmp_write_thread.cpp rtmp_write_thread.h rtmp_packet_handler.cpp rtmp_packet_handler.h rtmp_complex_hand_shake_v3.cpp rtmp_complex_hand_shake_v3.h rtmp_connection.cpp rtmp_connection.h rtmp_log.h)
add_executable(simple_rtmp_client ${SOURCE_FILES})

target_link_libraries(simple_rtmp_client ${PROJECT_SOURCE_DIR}/lib/libssl.a)
target_link_libraries(simple_rtmp_client ${PROJECT_SOURCE_DIR}/lib/libcrypto.a)

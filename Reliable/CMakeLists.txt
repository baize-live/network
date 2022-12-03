cmake_minimum_required(VERSION 3.23)
project(Reliable)

set(CMAKE_CXX_STANDARD 11)

add_executable(lab3-server lab3-server.cpp protocol/UDP.h protocol/RTP.h protocol/RTP.cpp protocol/protocol.h utils/FileUtil.h utils/FileUtil.cpp)
add_executable(lab3-client lab3-client.cpp protocol/UDP.h protocol/RTP.h protocol/RTP.cpp protocol/protocol.h utils/FileUtil.h utils/FileUtil.cpp)
add_executable(Test test.cpp utils/FileUtil.h utils/FileUtil.cpp)

target_link_libraries(lab3-server ws2_32)
target_link_libraries(lab3-client ws2_32)


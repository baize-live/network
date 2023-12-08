cmake_minimum_required(VERSION 3.23)
project(Reliable)

set(CMAKE_CXX_STANDARD 17)

link_directories(${PROJECT_SOURCE_DIR}/lib)

SET(RTP_SRC protocol_4/UDP.h protocol_4/RTP.h protocol_4/RTP.cpp)

add_library(RTP SHARED ${RTP_SRC})
target_link_libraries(RTP ws2_32)

add_executable(lab3-server lab3-server.cpp)
target_link_libraries(lab3-server ws2_32 RTP)

add_executable(lab3-client lab3-client.cpp)
target_link_libraries(lab3-client ws2_32 RTP)

add_executable(Test test.cpp)


cmake_minimum_required(VERSION 3.23)
project(Capture)

set(CMAKE_CXX_STANDARD 11)

#引入头文件
include_directories(${PROJECT_SOURCE_DIR}/include)
#引入库文件
link_directories(${PROJECT_SOURCE_DIR}/lib)
#
add_executable(Capture main.cpp parse_frame.cpp parse_frame.h)

target_link_libraries(Capture Packet wpcap ws2_32)

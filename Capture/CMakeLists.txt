cmake_minimum_required(VERSION 3.23)
project(Capture)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS -m32)

#引入头文件
include_directories(${PROJECT_SOURCE_DIR}/include)
#引入库文件
link_directories(${PROJECT_SOURCE_DIR}/libx86)
#
add_executable(Lab1 lab1.cpp parse_frame.cpp struct.h lab.cpp lab.h)
target_link_libraries(Lab1 Packet wpcap ws2_32)

add_executable(Lab3 lab3.cpp parse_frame.cpp produce_frame.cpp struct.h lab.cpp lab.h tool.cpp)
target_link_libraries(Lab3 Packet wpcap ws2_32)

add_executable(Lab5 lab5.cpp parse_frame.cpp produce_frame.cpp struct.h lab.cpp lab.h tool.cpp)
target_link_libraries(Lab5 Packet wpcap ws2_32)

add_executable(Test test.cpp parse_frame.cpp struct.h)
target_link_libraries(Test ws2_32)

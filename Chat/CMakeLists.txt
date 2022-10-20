cmake_minimum_required(VERSION 3.23)
project(Chat)

set(CMAKE_CXX_STANDARD 11)

add_executable(Server server/Server.cpp protocol/config.cpp)
add_executable(Client client/Client.cpp client/Model.cpp client/View.cpp client/Control.cpp protocol/config.cpp)

target_link_libraries(Server ws2_32)
target_link_libraries(Client ws2_32)

#ifndef RELIABLE_UDP_H
#define RELIABLE_UDP_H

#include <iostream>

#include "winsock2.h"

// UDP 基础协议
class UDP {
private:
    SOCKET udp_socket{};

    // 创建socket
    int create_socket() {
        WSADATA wsd{};
        // 初始化 socket环境 //第一个参数是dll版本号，第二个参数是创建的socket对象 需要确认是否初始化成功
        if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
            std::cout << "WSAStartup failure. Error = " << WSAGetLastError() << std::endl;
            return -1;
        }
        // 创建socket
        udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (udp_socket == INVALID_SOCKET) {
            std::cout << "create_socket failure. Error = " << WSAGetLastError() << std::endl;
            return -1;
        }
        return 0;
    }

    // 绑定socket
    int bind_socket(short port) const {
        // 创建套接字地址
        sockaddr_in socket_addr{};
        socket_addr.sin_family = AF_INET;
        socket_addr.sin_port = htons(port);//端口
        socket_addr.sin_addr.s_addr = inet_addr("127.0.0.1");//具体的地址
        if (bind(udp_socket, (sockaddr *) &socket_addr, sizeof(socket_addr)) == SOCKET_ERROR) {
            std::cout << "bind_socket failure. Error = " << WSAGetLastError() << std::endl;
            return -1;
        }
        return 0;
    }

public:
    UDP() = default;

    int init_socket() {
        if (create_socket() != 0) {
            return -1;
        }
        return 0;
    }

    int init_socket(short port) {
        if (create_socket() != 0 || bind_socket(port) != 0) {
            return -1;
        }
        return 0;
    }

    static sockaddr_in *get_SockAddr_In(const std::string &host, const short port) {
        // 配置远程主机信息
        auto *socket_addr = new sockaddr_in();
        socket_addr->sin_family = AF_INET;
        socket_addr->sin_port = htons(port);
        socket_addr->sin_addr.S_un.S_addr = inet_addr(host.c_str());
        return socket_addr;
    }

    int recv(char *buf, int len, sockaddr_in &socket_addr) const {
        int addr_len = sizeof(SOCKADDR);
        int now_len = recvfrom(udp_socket, buf, len, 0, (SOCKADDR *) &socket_addr, &addr_len);
        return now_len;
    }

    int send(char *buf, int len, sockaddr_in &socket_addr) const {
        int addr_len = sizeof(SOCKADDR);
        int now_len = sendto(udp_socket, buf, len, 0, (SOCKADDR *) &socket_addr, addr_len);
        return now_len;
    }

    ~UDP() {
        closesocket(udp_socket);
        WSACleanup();
    }
};

#endif //RELIABLE_UDP_H

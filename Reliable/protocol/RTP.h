#ifndef RELIABLE_RTP_H
#define RELIABLE_RTP_H

#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <random>
#include <iostream>
#include "winsock2.h"

#include "UDP.h"

using namespace std;
#define DATA_LEN 10000
#define DEBUG_RTP

#pragma pack(1)        // 进入字节对齐方式

// 1 Byte
struct RTP_Flag_t {
    BYTE ACK: 1;    // 确认
    BYTE SYN: 1;    // 建立连接
    BYTE FIN: 1;    // 断开连接
    BYTE RES1: 5;   // 保留
};

// 12 Bytes
struct RTP_Head_t {
    DWORD send_num;      // 发送序号
    DWORD recv_num;      // 接收序号
    WORD check_sum;      // 校验和
    RTP_Flag_t flag;     // 标志位
    BYTE windows_num;    // 窗口大小
};

// DATA_LEN Bytes
struct RTP_Data_t {
    BYTE data[DATA_LEN]; // 数据域
};

// 12 + DATA_LEN Bytes
struct RTP_Datagram_t {
    RTP_Head_t head;
    RTP_Data_t data;
};

// 窗口属性
struct Attribute_t {
    DWORD time;
    DWORD index;
    DWORD send_number;
    bool flag;
};

// 服务器状态枚举
enum ServerStatus {
    Server_init = 0,
    Server_listen,
    Server_accept,

    Server_Shake_Hands_2,
    Server_Connected,

    Server_Wave_Hands_2,
    Server_Connected_Close,

    Server_Unknown_Error,
};

// 客户端状态枚举
enum ClientStatus {
    Client_init = 0,

    Client_Shake_Hands_1,
    Client_Shake_Hands_3,

    Client_Connected,

    Client_Wave_Hands_1,
    Client_Wave_Hands_3,
    Client_Wave_Hands_5, // 等待一次网络延时, 若状态为改变则成功挥手

    Client_Connected_Close,

    Client_Unknown_Error,
};

#pragma pack()

// RTP 可靠传输基类
class RTP {
    // 随机数
    default_random_engine random;
protected:
    // UDP 工具
    UDP *udp_ptr = new UDP();
    // connect/accept 设置
    sockaddr_in *addr_ptr = nullptr;
    // 是否连接 提供给send 和 recv 使用
    bool isConnected = false;
    // 循环超时时间
    DWORD sleep_time = 0;
    // 窗口大小
    DWORD windows_number = 10;
    // 网络延迟 初始值1s
    DWORD network_delay = 1000;
    // 上次握手发送的时间
    DWORD Shake_Hands_Send_Time = -1;
    // 接收/发送基址
    DWORD recv_base = 0;
    DWORD send_base = random() % 100;
    // 接收/发送序号
    DWORD recv_number = 0;
    DWORD send_number = send_base;
    // 发送缓冲区
    vector<pair<int, RTP_Data_t>> send_buffers;
    // 发送缓冲区大小
    DWORD buffers_number = 0;
    DWORD free_buffer_head = 0;
    DWORD free_buffer_number = 0;
    DWORD valid_buffer_head = 0;
    // 发送窗口属性
    list<Attribute_t> send_windows_list;

    // 信号队列 用于接收发送线程间通信
    queue<RTP_Flag_t> signals_queue;

    // 接收缓冲区
    vector<char> recv_buffers;
    list<pair<int, int>> valid_buffer_index{{0, 0}};
    // 收发线程
    thread *thread_send = nullptr;
    thread *thread_recv = nullptr;
    thread *thread_resend = nullptr;

    // 对共享数据加锁
    mutex send_mutex;
    mutex recv_mutex;
    mutex test_mutex;

    // RTP 提供给子类的接口
    int recv_RTP_Datagram(RTP_Datagram_t *RTP_Datagram);

    int send_RTP_Signals(RTP_Datagram_t *RTP_Datagram, RTP_Head_t head);

    int send_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, RTP_Head_t head, DWORD index);

    void print_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, int len, const string &str);

    // 接收数据及缓冲区的处理
    void handle_recv_Data(RTP_Datagram_t *RTP_Datagram, int len);

    // 发送数据及缓冲区的处理
    void handle_send_Data(RTP_Datagram_t *RTP_Datagram);

    // 重传数据及缓冲区的处理
    void handle_resend_Data(RTP_Datagram_t *RTP_Datagram);

    // 禁止外部构建
    RTP() = default;

public:
    // RTP 提供给上层的接口
    int recv(char *buf, int len, int timeout = INT32_MAX);

    int send(char *buf, int len, int timeout = INT32_MAX);

    ~RTP() {
        delete udp_ptr;
        delete addr_ptr;
        delete thread_recv;
        delete thread_send;
        delete thread_resend;
    }
};

class RTP_Server : public RTP {
private:
    // 服务端状态
    ServerStatus Status;

    void recv_thread();

    void send_thread();

    void resend_thread();

public:
    int server(short port) {
        int len = udp_ptr->init_socket(port);
        Status = (len == 0 ? Server_init : Server_Unknown_Error);
        return len;
    }

    int listen() {
        // 启动接收和发送线程
        thread_recv = new thread(&RTP_Server::recv_thread, this);
        thread_send = new thread(&RTP_Server::send_thread, this);
        thread_recv->detach();
        thread_send->detach();
        // 初始化 对方地址
        addr_ptr = new sockaddr_in();
        // 服务器开始监听
        Status = Server_listen;
        return 0;
    }

    int accept() {
        // 根据状态判断 是否连接成功
        while (true) {
            switch (Status) {
                case Server_accept:
                case Server_Shake_Hands_2:
                    Sleep(sleep_time);
                    continue;
                case Server_listen:
                    Status = Server_accept;
                    continue;
                case Server_init:
                case Server_Unknown_Error:
                    return -1;
                default:
                    return 0;
            }
        }
    }

    ~RTP_Server() {
        while (!signals_queue.empty()) {
            Sleep(sleep_time);
        }
        Status = Server_Connected_Close;
    }
};

class RTP_Client : public RTP {
private:
    // 客户端状态
    ClientStatus Status;

    void recv_thread();

    void send_thread();

    void resend_thread();

public:
    int client() {
        int len = udp_ptr->init_socket();
        Status = (len == 0 ? Client_init : Client_Unknown_Error);
        return len;
    }

    int connect(const string &host, short port) {
        // 启动接收和发送线程
        thread_recv = new thread(&RTP_Client::recv_thread, this);
        thread_send = new thread(&RTP_Client::send_thread, this);
        thread_recv->detach();
        thread_send->detach();
        // 初始化
        addr_ptr = UDP::get_SockAddr_In(host, port);

        // 开始计时 超时连接失败
        DWORD start = GetTickCount();
        // 根据状态判断 是否连接成功
        while (true) {
            switch (Status) {
                case Client_init:
                    Status = Client_Shake_Hands_1;
                    break;
                case Client_Shake_Hands_1:
                case Client_Shake_Hands_3:
                    Sleep(sleep_time);
                    break;
                case Client_Unknown_Error:
                    return -1;
                default:
                    return 0;
            }
            if (GetTickCount() - start > network_delay * 10) {
                return -1;
            }
        }
    }

    int close() {
        while (free_buffer_number != buffers_number) {
            Sleep(1000);
        }

        // 客户端准备连接
        Status = Client_Wave_Hands_1;
        DWORD start = GetTickCount();
        // 根据状态判断 是否连接成功
        while (true) {
            switch (Status) {
                case Client_Connected_Close:
                    return 0;
                case Client_Unknown_Error:
                    return -1;
                default:
                    Sleep(sleep_time);
            }
            if (GetTickCount() - start > network_delay * 10) {
                Status = Client_Unknown_Error;
                return -1;
            }
        }
    }

    ~RTP_Client() {
        Status = Client_Connected_Close;
    }
};

#endif //RELIABLE_RTP_H
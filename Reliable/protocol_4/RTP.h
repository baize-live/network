#ifndef RELIABLE_RTP_H
#define RELIABLE_RTP_H

#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <random>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "winsock2.h"

#include "UDP.h"

using namespace std;
#define DATA_LEN 10000

#pragma pack(1)        // 进入字节对齐方式

// 1 Byte
struct RTP_Flag_t {
    BYTE ACK: 1;    // 确认
    BYTE SYN: 1;    // 建立连接
    BYTE FIN: 1;    // 断开连接
    BYTE RES1: 5;   // 保留
};

// 16 Bytes
struct RTP_Head_t {
    DWORD send_num;      // 发送序号
    DWORD recv_num;      // 接收序号
    DWORD data_len;      // 发送长度
    WORD check_sum;      // 校验和
    RTP_Flag_t flag;     // 标志位
    BYTE windows_num;    // 窗口大小
};

// DATA_LEN Bytes
struct RTP_Data_t {
    BYTE data[DATA_LEN]; // 数据域
};

// 16 + DATA_LEN Bytes
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

    Server_Close,

    Server_Unknown_Error,
};

// 客户端状态枚举
enum ClientStatus {
    Client_create = 0,
    Client_init,

    // Common_Client
    Client_Send_Shake_Hands_1,
    Client_Send_Shake_Hands_3,

    // Server_Client
    Client_Send_Shake_Hands_2,

    Client_Connected,

    Client_Send_Wave_Hands_1,
    Client_Send_Wave_Hands_23,
    Client_Send_Wave_Hands_4,

    Client_Wave_Hands_delay, // 等待一次网络延时, 若状态为改变则成功挥手

    Client_Connected_Close,

    Client_Unknown_Error,
};

// 客户端类型枚举
enum ClientType {
    Common_Client,
    Server_Client
};

#pragma pack()

class RTP_Server;

class RTP_Client {
    // 随机数
    default_random_engine random;
protected:
    // UDP 工具
    UDP *udp_ptr = new UDP();
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

    void recv_thread();

    void send_thread();

    void resend_thread();

    // 子类重写收发逻辑
    virtual int recv_RTP_Datagram(RTP_Datagram_t *RTP_Datagram);

    virtual int send_RTP_Signals(RTP_Datagram_t *RTP_Datagram, RTP_Head_t head);

    virtual int send_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, RTP_Head_t head, DWORD index);

public:
    // Client类型
    ClientType Type = Common_Client;
    // 客户端状态
    ClientStatus Status = Client_create;
    // connect 设置
    sockaddr_in *addr_ptr = nullptr;

    RTP_Client() = default;

    int recv(char *buf, int len, int timeout = INT32_MAX);

    int send(char *buf, int len, int timeout = INT32_MAX);

    void init();

    int connect(const string &host, short port, DWORD time = 5);

    int close(DWORD time = 5);

    virtual ~RTP_Client() {
        delete udp_ptr;
        delete addr_ptr;
        delete thread_recv;
        delete thread_send;
        delete thread_resend;
    }

};

class RTP_Server {
private:
    class RTP_Server_Client : public RTP_Client {
        RTP_Server *server_ptr = nullptr;

        int recv_RTP_Datagram(RTP_Datagram_t *RTP_Datagram) override;

        int send_RTP_Signals(RTP_Datagram_t *RTP_Datagram, RTP_Head_t head) override;

        int send_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, RTP_Head_t head, DWORD index) override;

    public:
        queue<RTP_Datagram_t> recv_queue;

        RTP_Server_Client(RTP_Server *server_ptr, sockaddr_in *addr) : server_ptr(server_ptr) {
            Type = Server_Client;
            Status = Client_init;
            addr_ptr = new sockaddr_in();
            addr_ptr->sin_family = AF_INET;
            addr_ptr->sin_port = addr->sin_port;
            addr_ptr->sin_addr = addr->sin_addr;

            // 启动接收和发送线程
            thread_recv = new thread(&RTP_Server::RTP_Server_Client::recv_thread, this);
            thread_send = new thread(&RTP_Server::RTP_Server_Client::send_thread, this);
            thread_recv->detach();
            thread_send->detach();
        }

        ~RTP_Server_Client() override = default;

    };

    // 服务端状态
    ServerStatus Status;
    // 未连接的客户端
    unordered_map<UINT64, RTP_Client *> clients_disconnected;
    // 连接的客户端
    unordered_map<UINT64, RTP_Client *> clients_connected;

    mutex client_mutex;

    // UDP 工具
    UDP *udp_ptr = new UDP();
    // 循环等待时间
    const DWORD sleep_time = 0;
    // 客户端发送报文队列
    queue<pair<RTP_Client *, RTP_Datagram_t>> send_queue;

    void recv_thread();

    void send_thread();

    void loop_thread();

public:

    int server(short port) {
        int len = udp_ptr->init_socket(port);
        Status = (len == 0 ? Server_init : Server_Unknown_Error);
        return len;
    }

    int listen();

    RTP_Client *accept();

    void close();

    ~RTP_Server() {
        if (Status != Server_Close) {
            close();
        }
        delete udp_ptr;
    }
};

#endif //RELIABLE_RTP_H
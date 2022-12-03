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
#define DATA_LEN 1440
#define buf_len 4096
#define DEBUG 1

#pragma pack(1)        // 进入字节对齐方式

// 1 Bytes
struct Flag {
    BYTE ACK: 1;    // 确认
    BYTE SYN: 1;    // 建立连接
    BYTE FIN: 1;    // 断开连接
    BYTE RES1: 5;   // 保留
};

// 1452 Bytes
struct RTP_Datagram_t {
    DWORD send_number;   // 发送序号
    DWORD recv_number;   // 接收序号
    WORD sum_check;      // 校验和
    Flag flag;           // 标志位
    BYTE windows_size;   // 窗口大小
    BYTE data[DATA_LEN]; // 数据域
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

// 窗口属性
struct Attribute {
    int begin;
    int length;
    int send_number;
    DWORD time;
    bool flag;
};

#pragma pack()

class RTP {
protected:
    // ============================= 初始设置 ==================== //
    // UDP工具
    UDP *udp_ptr = new UDP();
    // 对方的addr connect或者accept时设置
    sockaddr_in *addr_ptr = nullptr;
    // 循环超时时间
    DWORD sleep_time = 0;
    // 发送序号
//    DWORD send_number = rand() % 100;
    DWORD send_number = 0;
    // 发送缓冲区有效字节
    pair<int, int> send_buf_effective_index{0, 0};
    int send_buf_effective_num = 0;
    // ============================= 握手更新设置 ==================== //
    // 是否连接 提供给send 和 recv 使用
    bool isConnected = false;
    // 上次握手发送的时间
    DWORD Shake_Hands_Send_Time = -1;
    // 窗口大小
    DWORD windows_size = 4;
    // 网络延迟 ms
    DWORD network_delay = 1000;
    // 接收序号
    DWORD recv_number = 0;
    // 已确认数据基址
    DWORD base_index = 0;
    // 发送缓冲区 根据双方最小窗口设置
    vector<char> send_buf;
    int send_buf_size = 0;
    // 接收缓冲区 根据双方最小窗口设置
    vector<char> recv_buf;
    int recv_buf_size = 0;
    // 接收缓冲有效字节
    vector<pair<int, int>> recv_buf_effective_index;
    // ============================= 无初始值 ==================== //
    // 临时缓冲区
    char temp_buf[DATA_LEN]{};
    // 收发线程
    thread *thread_send = nullptr;
    thread *thread_recv = nullptr;
    thread *thread_resend = nullptr;

    // 对共享数据加锁
    mutex send_mutex;
    mutex recv_mutex;
    mutex send_window_mutex;
    mutex share_data_mutex;

    // ======== 提供一些get set 方法 方便锁的管理 TODO:锁的粒度太大 ======== //
    DWORD get_send_number() {
        // 创建时加锁 销毁时解锁
        unique_lock<mutex> lock(share_data_mutex);
        return send_number;
    }

    void add_send_number(DWORD number) {
        // 创建时加锁 销毁时解锁
        unique_lock<mutex> lock(share_data_mutex);
        send_number += number;
    }

    int get_effective_index_len() {
        // 创建时加锁 销毁时解锁
        unique_lock<mutex> lock(share_data_mutex);
        return send_buf_effective_num;
    }

    void set_send_buf_effective_index_first(int begin, int length) {
        unique_lock<mutex> lock(share_data_mutex);
        send_buf_effective_num -= (begin + length - send_buf_effective_index.first);
        send_buf_effective_index.first = begin + length;
        if (send_buf_effective_index.first >= send_buf_size) {
            send_buf_effective_index.first -= (int) send_buf_size;
        }
    }

    int get_send_buf_effective_index_first() {
        unique_lock<mutex> lock(share_data_mutex);
        return send_buf_effective_index.first;
    }

    int get_send_buf_effective_index_second() {
        unique_lock<mutex> lock(share_data_mutex);
        return send_buf_effective_index.second;
    }

    void add_send_buf_effective_index_second(int number) {
        unique_lock<mutex> lock(share_data_mutex);
        send_buf_effective_num += number;
        send_buf_effective_index.second += number;
        if (send_buf_effective_index.second >= send_buf_size) {
            send_buf_effective_index.second -= send_buf_size;
        }
    }

    int get_base_index() {
        unique_lock<mutex> lock(share_data_mutex);
        return base_index;
    }

    void add_index_offset(int length) {
        unique_lock<mutex> lock(share_data_mutex);
        base_index += length;
        cout << "base_index: " << base_index << endl;
    }

    // ============================= Utils ==================== //
    // 生成校验码
    static WORD generate_sum_check(RTP_Datagram_t *RTP_Datagram);

    // 检查校验码
    static bool check_sum_check(RTP_Datagram_t *RTP_Datagram);

    // 生成RTP_Datagram_t 报文
    int generate_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, char *buf, int len,
                              BYTE ACK, BYTE SYN, BYTE FIN, DWORD number) const;

    // 发送信号
    int send_signals(RTP_Datagram_t *RTP_Datagram, byte ack, byte syn, byte fin);

    // 给定一个 buf 有效索引, 将数据填充到temp_buf 里面 并发送
    int send_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, DWORD number, int begin, int length);

    // ============================= 子类重写 ==================== //
    virtual void recv_thread() = 0;

    virtual void send_thread() = 0;

public:

    int recv(char *buf, int len, int timeout = INT32_MAX) {
        DWORD start = GetTickCount();
        while (true) {
            // 读超时
            if (!isConnected || GetTickCount() - start >= timeout) {
                return -1;
            }

            if (recv_buf_effective_index[0].second == 0) {
                Sleep(sleep_time);
                continue;
            }

            // 操作共享数据前加锁
            recv_mutex.lock();

            int data_len = (recv_buf_effective_index[0].second < len) ? (int) recv_buf_effective_index[0].second : len;
            // 拷贝元素
            copy(recv_buf.cbegin(), recv_buf.cbegin() + data_len, buf);
            // 调整缓冲
            copy(recv_buf.begin() + data_len, recv_buf.end(), recv_buf.begin());
            // 调整缓冲索引
            base_index += data_len;
            for (auto &pair: recv_buf_effective_index) {
                pair.first -= data_len;
                pair.second -= data_len;
            }
            recv_buf_effective_index[0].first = 0;

            // 操作共享数据后解锁
            recv_mutex.unlock();
            return data_len;
        }
    }

    int send(char *buf, int len, int timeout = INT32_MAX) {
        DWORD start = GetTickCount();
        while (true) {
            // 写超时
            if (!isConnected || GetTickCount() - start >= timeout) {
                return -1;
            }

            send_mutex.lock();
            // 缓冲区数据满了
            if ((send_buf_size - get_effective_index_len()) < len) {
                send_mutex.unlock();
                Sleep(sleep_time);
                continue;
            }

            // 确保此刻send_buf_effective_index.second的值正确
            int temp_send_buf_effective_index_second = get_send_buf_effective_index_second();
            // 拷贝元素
            int buf_end_2_effective_index_end = (int) send_buf_size - temp_send_buf_effective_index_second;
            if (buf_end_2_effective_index_end > len) {
                copy(buf, buf + len, send_buf.begin() + temp_send_buf_effective_index_second);
            } else {
                copy(buf, buf + buf_end_2_effective_index_end, send_buf.begin() + temp_send_buf_effective_index_second);
                copy(buf + buf_end_2_effective_index_end, buf + len, send_buf.begin());
            }
            // 调整缓冲索引
            add_send_buf_effective_index_second(len);
            send_mutex.unlock();

            return len;
        }
    }

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
    ServerStatus Statu;
    // 信号队列 用于接收发送线程间通信
    queue<Flag> signals_queue;

    void recv_thread();

    void send_thread();

public:
    int server(short port) {
        int len = udp_ptr->init_socket(port);
        Statu = (len == 0 ? Server_init : Server_Unknown_Error);
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
        Statu = Server_listen;
        return 0;
    }

    int accept() {
        // 根据状态判断 是否连接成功
        while (true) {
            switch (Statu) {
                case Server_accept:
                case Server_Shake_Hands_2:
                    Sleep(sleep_time);
                    continue;
                case Server_listen:
                    Statu = Server_accept;
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
        Statu = Server_Connected_Close;
    }
};

class RTP_Client : public RTP {
private:
    // 客户端状态
    ClientStatus Statu;
    // 发送窗口属性
    list<Attribute> send_window;

    void recv_thread();

    void send_thread();

    void resend_thread();

public:
    int client() {
        int len = udp_ptr->init_socket();
        Statu = (len == 0 ? Client_init : Client_Unknown_Error);
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
            switch (Statu) {
                case Client_init:
                    Statu = Client_Shake_Hands_1;
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
        while (get_effective_index_len() != 0) {
            Sleep(1000);
        }

        // 客户端准备连接
        Statu = Client_Wave_Hands_1;
        DWORD start = GetTickCount();
        // 根据状态判断 是否连接成功
        while (true) {
            switch (Statu) {
                case Client_Connected_Close:
                    return 0;
                case Client_Unknown_Error:
                    return -1;
                default:
                    Sleep(sleep_time);
            }
            if (GetTickCount() - start > network_delay * 10) {
                Statu = Client_Unknown_Error;
                return -1;
            }
        }
    }

    ~RTP_Client() {
        Statu = Client_Connected_Close;
    }
};

#endif //RELIABLE_RTP_H
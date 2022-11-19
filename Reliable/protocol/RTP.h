#ifndef RELIABLE_RTP_H
#define RELIABLE_RTP_H

#include <mutex>
#include <thread>
#include <random>
#include <iostream>
#include "winsock2.h"

#include "UDP.h"

using namespace std;
#define DATA_LEN 1280
#define buf_len 4096

#pragma pack(1)        // 进入字节对齐方式

struct RTP_Flag_t {
    BYTE ACK: 1;    // 确认
    BYTE SYN: 1;    // 建立连接
    BYTE FIN: 1;    // 断开连接
    BYTE RES1: 5;   // 保留
    BYTE RES2;      // 保留
};

struct RTP_Datagram_t {
    DWORD send_number;   // 发送序号
    DWORD recv_number;   // 接收序号
    WORD sum_check;
    RTP_Flag_t flag;
    BYTE data[DATA_LEN];    // 数据
};

#pragma pack()

// 传输层协议 3.1
class RTP {
    UDP *udp_ptr;
    // 表示该类是客户端还是服务器
    bool isServer{};
    // 表示是否握手成功
    bool isConnected;
    // 对方的addr
    sockaddr_in *addr_ptr;
    // 窗口大小
    unsigned windows_size;
    // 循环超时时间
    unsigned sleep_time;
    // 网络延迟 定时器
    unsigned network_delay;

    // 对共享数据加锁
    mutex send_mutex;
    mutex recv_mutex;

    // ============================= 加锁 ==================== //
    // 发送序号
    DWORD send_number;
    // 发送缓冲有效字节
    unsigned send_buf_effective_index;
    // 发送缓冲区
    vector<char> send_buf;
    // 临时发送缓冲区
    char temp_send_buf[buf_len]{};

    // 接收序号
    DWORD recv_number;
    // 接收缓冲区相对于 recv_number 的偏移
    unsigned recv_buf_index_offset;
    // 接收缓冲有效字节
    vector<pair<unsigned, unsigned>> recv_buf_effective_index;
    // 接收缓冲区
    vector<char> recv_buf;
    // ============================= 加锁 ==================== //

    // 临时缓冲区
    char temp_buf[buf_len]{};

    thread *thread_send;
    thread *thread_recv;

    // 生成校验码
    static WORD generate_sum_check(RTP_Datagram_t *RTP_Datagram) {
        WORD *get_sum_check = (WORD *) RTP_Datagram;
        int times = sizeof(RTP_Datagram_t) / sizeof(WORD);
        DWORD temp = get_sum_check[0];
        for (int i = 1; i < times; ++i) {
            temp += get_sum_check[i];
            if (temp > 0xFFFF) {
                temp += (temp >> 16);
                temp &= 0xFFFF;
            }
        }
        return temp ^ 0xFFFF;
    }

    // 检查校验码
    static bool check_sum_check(RTP_Datagram_t *RTP_Datagram) {
        WORD *get_sum_check = (WORD *) RTP_Datagram;
        int times = sizeof(RTP_Datagram_t) / sizeof(WORD);
        DWORD temp = get_sum_check[0];
        for (int i = 1; i < times; ++i) {
            temp += get_sum_check[i];
            if (temp > 0xFFFF) {
                temp += (temp >> 16);
                temp &= 0xFFFF;
            }
        }
        return (temp & 0xFFFF) == 0xFFFF;
    }

    // 生成RTP_Datagram_t 报文
    RTP_Datagram_t *
    generate_RTP_Datagram(BYTE ACK, BYTE SYN, BYTE FIN, char *buf, int len, int &RTP_Datagram_len) const {
        auto *RTP_Datagram = new RTP_Datagram_t();
        memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
        RTP_Datagram->send_number = send_number;
        RTP_Datagram->recv_number = recv_number;
        RTP_Datagram->flag.ACK = ACK;
        RTP_Datagram->flag.SYN = SYN;
        RTP_Datagram->flag.FIN = FIN;
        memcpy(RTP_Datagram->data, buf, len);
        // 计算检验和
        RTP_Datagram->sum_check = generate_sum_check(RTP_Datagram);
        RTP_Datagram_len = (int) sizeof(RTP_Datagram_t) - DATA_LEN + len;
        return RTP_Datagram;
    }

    // 回包
    int reply_ack(byte ack, byte syn, byte fin) {
        // 回包
        int len = 0;
        auto *ACK_RTP_Datagram = generate_RTP_Datagram(ack, syn, fin, temp_buf, 0, len);
        len = udp_ptr->send((char *) ACK_RTP_Datagram, len, *addr_ptr);
        delete[] ACK_RTP_Datagram;
        return len;
    }

    // 重发线程
    void send_again_thread(RTP_Datagram_t *RTP_Datagram, int len, bool &ack_flag) {
        DWORD start = GetTickCount();
        while (!ack_flag) {
            if (GetTickCount() - start >= network_delay * 5) {
                udp_ptr->send((char *) RTP_Datagram, len, *addr_ptr);
                start = GetTickCount();
                cerr << "丢包 " << RTP_Datagram->send_number << " 重传... " << endl;
            }
        }
    }

    void recv_thread() {
        int len;
        auto *RTP_Datagram = new RTP_Datagram_t();

        // 将数据存到接收缓冲区
        while (isConnected) {
            memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
            len = udp_ptr->recv((char *) RTP_Datagram, sizeof(RTP_Datagram_t), *addr_ptr);
            if (len != -1 && check_sum_check(RTP_Datagram)) {
                // 考虑是不是重传
                if (RTP_Datagram->send_number < recv_number) {
                    // 回包
                    reply_ack(1, 0, 0);
                    continue;
                }

                cout << "接收 " << RTP_Datagram->send_number << " 成功" << endl;

                /* 将数据 放在接收缓冲区中 考虑缓冲区满的情况 */

                // 数据域的长度
                unsigned data_len = len - (sizeof(RTP_Datagram_t) - DATA_LEN);

                // 获得缓冲区的起始位置
                unsigned start, end;
                while (true) {
                    // 操作共享数据前加锁
                    recv_mutex.lock();

                    start = RTP_Datagram->send_number - recv_buf_index_offset;
                    end = start + data_len;

                    // 缓冲区可以放下就跳出
                    if (end <= recv_buf.size()) {
                        break;
                    }
                    // 缓冲区放不下就开锁, 等待上层调用
                    recv_mutex.unlock();
                    Sleep(sleep_time);
                }

                // 拷贝数据域到缓冲区
                copy(RTP_Datagram->data, RTP_Datagram->data + data_len, recv_buf.begin() + start);

                // 将新的片段插入 recv_buf_effective_index 中
                if (recv_buf_effective_index.size() == 1) {
                    recv_buf_effective_index.emplace_back(pair<unsigned, unsigned>(start, end));
                } else {
                    for (int i = 1; i < recv_buf_effective_index.size(); i++) {
                        if (start >= recv_buf_effective_index[i - 1].second
                            && end <= recv_buf_effective_index[i].first) {
                            recv_buf_effective_index.insert(
                                    recv_buf_effective_index.begin() + i,
                                    pair<unsigned, unsigned>(start, end)
                            );
                            break;
                        }
                    }
                }

                // 合并 recv_buf_effective_index 中重复的索引
                for (int i = 1; i < recv_buf_effective_index.size(); i++) {
                    if (recv_buf_effective_index[i].first <= recv_buf_effective_index[i - 1].second) {
                        recv_buf_effective_index[i - 1].second = recv_buf_effective_index[i].second;
                        recv_buf_effective_index.erase(recv_buf_effective_index.begin() + i);
                        --i;
                    }
                }

                // 接收数调整
                recv_number = recv_buf_index_offset + recv_buf_effective_index[0].second;

                // 操作共享数据后解锁
                recv_mutex.unlock();

                // 回包
                reply_ack(1, 0, 0);
            }
        }

        delete RTP_Datagram;
    }

    void send_thread() {
        // 将发送缓冲区的数据发送出去
        while (isConnected) {
            int len = 0;
            int send_len = (send_buf_effective_index < DATA_LEN) ? (int) send_buf_effective_index : DATA_LEN;

            if (send_len == 0) {
                Sleep(sleep_time);
                continue;
            }

            // 操作共享数据前加锁
            send_mutex.lock();

            // 拷贝数据并发送
            copy(send_buf.begin(), send_buf.begin() + send_len, temp_send_buf);
            auto *RTP_Datagram_send = generate_RTP_Datagram(0, 0, 0, temp_send_buf, send_len, len);
            udp_ptr->send((char *) RTP_Datagram_send, len, *addr_ptr);

            // 启动 定时器
            bool ack_flag = false;
            thread timer(&RTP::send_again_thread, this, RTP_Datagram_send, len, ref(ack_flag));
            timer.detach();

            auto *RTP_Datagram = new RTP_Datagram_t();
            memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
            len = udp_ptr->recv((char *) RTP_Datagram, sizeof(RTP_Datagram_t), *addr_ptr);

            // 收到ACK响应包 清空 RTP_Datagram_send
            ack_flag = true;
            delete RTP_Datagram_send;

            if (len == -1) {
                delete RTP_Datagram;
                // 操作共享数据后解锁
                send_mutex.unlock();
                continue;
            } else {
                if (!check_sum_check(RTP_Datagram) || RTP_Datagram->flag.ACK != 1 ||
                    RTP_Datagram->recv_number != send_number + send_len) {
                    delete RTP_Datagram;
                    // 操作共享数据后解锁
                    send_mutex.unlock();
                    continue;
                } else {
                    cout << "发送" << RTP_Datagram->recv_number << " 成功" << endl;
                    // 调整收发数 以及 缓冲区
                    send_number += send_len;
                    copy(send_buf.begin() + send_len, send_buf.end(), send_buf.begin());
                    send_buf_effective_index -= send_len;

                    delete RTP_Datagram;
                    // 操作共享数据后解锁
                    send_mutex.unlock();
                }
            }
        }
    }

public:
    RTP() {
        random_device rd;
        // ========== 产生一个随机数
        udp_ptr = nullptr;
        addr_ptr = nullptr;
        isConnected = false;
        windows_size = 1;
        sleep_time = 0;
        network_delay = 10;
        send_number = rd() % 100;
        recv_number = 0;
        recv_buf_index_offset = 0;
        send_buf_effective_index = 0;
        recv_buf_effective_index.emplace_back(0, 0);
        send_buf.resize(DATA_LEN * windows_size);
        recv_buf.resize(DATA_LEN * windows_size);
        thread_send = nullptr;
        thread_recv = nullptr;
    }

    int server(short port) {
        isServer = true;
        udp_ptr = new UDP();
        return udp_ptr->init_socket(port);
    }

    int client() {
        isServer = false;
        udp_ptr = new UDP();
        return udp_ptr->init_socket();
    }

    int connect(const string &host, short port) {
        if (isServer) {
            return -1;
        }
        addr_ptr = UDP::get_SockAddr_In(host, port);

        // =============== 开始握手
        int len;
        auto *RTP_Datagram = new RTP_Datagram_t();

        // 握手第一次
        DWORD start = GetTickCount();
        if (reply_ack(0, 1, 0) == -1) {
            return -1;
        }
        cout << "client 第一次握手 成功 " << endl;

        // 握手第二次
        memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
        len = udp_ptr->recv((char *) RTP_Datagram, sizeof(RTP_Datagram_t), *addr_ptr);
        if (len == -1) {
            delete RTP_Datagram;
            return -1;
        }
        if (!check_sum_check(RTP_Datagram) || RTP_Datagram->flag.ACK != 1 || RTP_Datagram->flag.SYN != 1) {
            delete RTP_Datagram;
            return -1;
        }
        recv_number = RTP_Datagram->send_number;
        recv_buf_index_offset = RTP_Datagram->send_number;
        network_delay = (GetTickCount() - start) > network_delay ? (GetTickCount() - start) : network_delay;
        delete RTP_Datagram;
        cout << "client 第二次握手 成功 " << endl;

        // 握手第三次
        if (reply_ack(1, 1, 0) == -1) {
            return -1;
        }
        cout << "client 第三次握手 成功 " << endl;

        // 握手成功后, 启动独立线程用于发送数据
        isConnected = true;
        thread_send = new thread(&RTP::send_thread, this);
        thread_send->detach();
        return 0;
    }

    int accept() {
        if (!isServer) {
            return -1;
        }
        addr_ptr = new sockaddr_in();

        // =============== 开始握手
        int len;
        auto *RTP_Datagram = new RTP_Datagram_t();

        // 握手第一次
        memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
        len = udp_ptr->recv((char *) RTP_Datagram, sizeof(RTP_Datagram_t), *addr_ptr);

        if (len == -1) {
            delete RTP_Datagram;
            return -1;
        }
        if (!check_sum_check(RTP_Datagram) || RTP_Datagram->flag.SYN != 1) {
            delete RTP_Datagram;
            return -1;
        }
        recv_number = RTP_Datagram->send_number;
        recv_buf_index_offset = RTP_Datagram->send_number;
        cout << "server 第一次握手 成功 " << endl;

        // 握手第二次
        DWORD start = GetTickCount();
        if (reply_ack(1, 1, 0) == -1) {
            return -1;
        }
        cout << "server 第二次握手 成功 " << endl;

        // 握手第三次
        memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
        len = udp_ptr->recv((char *) RTP_Datagram, sizeof(RTP_Datagram_t), *addr_ptr);
        if (len == -1) {
            delete RTP_Datagram;
            return -1;
        }
        if (!check_sum_check(RTP_Datagram) || RTP_Datagram->flag.ACK != 1 || RTP_Datagram->flag.SYN != 1) {
            delete RTP_Datagram;
            return -1;
        }
        network_delay = (GetTickCount() - start) > network_delay ? (GetTickCount() - start) : network_delay;
        delete RTP_Datagram;
        cout << "server 第二次握手 成功 " << endl;

        // 握手成功后, 启动独立线程用于接收数据
        isConnected = true;
        thread_recv = new thread(&RTP::recv_thread, this);
        thread_recv->detach();
        return 0;
    }

    int recv(char *buf, int len, int timeout = INT32_MAX) {
        DWORD start = GetTickCount();
        while (true) {
            // 读超时
            if (GetTickCount() - start >= timeout) {
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
            recv_buf_index_offset += data_len;
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
            if (GetTickCount() - start >= timeout) {
                return -1;
            }
            // TODO: 重写 避免缓冲区太小导致无法发送数据
            if ((send_buf.size() - send_buf_effective_index) < len) {
                Sleep(sleep_time);
                continue;
            }
            // 操作共享数据前加锁
            send_mutex.lock();
            // 拷贝元素
            copy(buf, buf + len, send_buf.begin() + send_buf_effective_index);
            // 调整缓冲索引
            send_buf_effective_index += len;
            // 操作共享数据后解锁
            send_mutex.unlock();
            return len;
        }
    }

    int recv(vector<char> &buf, int timeout = INT32_MAX) {
        int len = recv(temp_buf, buf.size(), timeout);
        if (len != -1) {
            copy(temp_buf, temp_buf + len, buf.begin());
        }
        return len;
    }

    int send(vector<char> &buf, int timeout = INT32_MAX) {
        copy(buf.begin(), buf.end(), temp_buf);
        return send(temp_buf, buf.size(), timeout);
    }

    int close() {
        isConnected = false;
        Sleep(sleep_time * 3);
        return 0;
    }

    ~RTP() {
        delete udp_ptr;
        delete addr_ptr;
        delete thread_recv;
        delete thread_send;
    }

};

#endif //RELIABLE_RTP_H

#include <shared_mutex>
#include "RTP.h"

mutex test_mutex;
#define DEBUG_RTP

// ================================================================================================================ //
// RTP_Client 提供给上层的接口
int RTP_Client::recv(char *buf, int len, int timeout) {
    DWORD start = GetTickCount();
    while (true) {
        // 读超时
        if (!isConnected || GetTickCount() - start >= timeout) {
            return -1;
        }

        // 无数据
        if (valid_buffer_index.begin()->second == 0) {
            Sleep(sleep_time);
            continue;
        }

        // 读数据加锁
        int length;
        {
            unique_lock<mutex> lock(recv_mutex);

            // 有数据
            length = min(len, valid_buffer_index.begin()->second);
            copy(recv_buffers.begin(), recv_buffers.begin() + length, buf);
            copy(recv_buffers.begin() + length, recv_buffers.end(), recv_buffers.begin());

            // 更新索引
            recv_base += length;
            for (auto &item: valid_buffer_index) {
                item.first -= length;
            }
            valid_buffer_index.begin()->first = 0;
            valid_buffer_index.begin()->second -= length;
        }

        return length;
    }
}

int RTP_Client::send(char *buf, int len, int timeout) {
    int offset = 0;
    DWORD start = GetTickCount();
    while (offset < len) {
        // 写超时
        if (!isConnected || GetTickCount() - start >= timeout) {
            return -1;
        }

        // 缓冲区已满
        if (free_buffer_number == 0) {
            Sleep(sleep_time);
            continue;
        }

        // 智能发送锁
        {
            unique_lock<mutex> lock(send_mutex);
            // 拷贝数据
            int length = len < DATA_LEN ? len : DATA_LEN;
            send_buffers[free_buffer_head].first = length;
            memcpy(send_buffers[free_buffer_head].second.data, buf + offset, length);

            // 调整索引
            offset += length;
            ++free_buffer_head;
            free_buffer_head %= buffers_number;
            --free_buffer_number;
        }
    }
    return len;
}

void RTP_Client::init() {
    int len = udp_ptr->init_socket();
    Status = (len == 0 ? Client_init : Client_Unknown_Error);
}

int RTP_Client::connect(const string &host, short port, DWORD time) {
    // 初始化
    addr_ptr = UDP::get_SockAddr_In(host, port);
    // 启动接收和发送线程
    thread_recv = new thread(&RTP_Client::recv_thread, this);
    thread_send = new thread(&RTP_Client::send_thread, this);
    thread_recv->detach();
    thread_send->detach();

    // 开始计时 超时连接失败
    DWORD start = GetTickCount();
    time *= network_delay;
    // 根据状态判断 是否连接成功
    while (true) {
        switch (Status) {
            // 初始化状态 可以转换为握手态
            case Client_init:
                Status = Client_Send_Shake_Hands_1;
                break;
            case Client_Send_Shake_Hands_1:
            case Client_Send_Shake_Hands_3:
                Sleep(sleep_time);
                break;
            case Client_Connected:
                return 0;
            default:
                Status = Client_Unknown_Error;
                return -1;
        }
        if (GetTickCount() - start > time) {
            if (Status == Client_Connected) {
                return 0;
            }
            return -1;
        }
    }
}

int RTP_Client::close(DWORD time) {
    while (free_buffer_number != buffers_number) {
        Sleep(1000);
    }

    // 客户端准备连接
    Status = Client_Send_Wave_Hands_1;
    DWORD start = GetTickCount();
    time *= network_delay;
    int flag = 0;
    // 根据状态判断 是否连接成功
    while (true) {
        switch (Status) {
            case Client_Connected_Close:
                flag = 1;
                break;
            case Client_Unknown_Error:
                flag = -1;
                break;
            default:
                Sleep(sleep_time);
        }
        if (flag != 0) {
            break;
        }
        if (GetTickCount() - start > time) {
            Status = Client_Unknown_Error;
            flag = -1;
            break;
        }
    }

    return flag;
}

// ================================================================================================================ //
// RTP_Server 提供给上层的接口
int RTP_Server::listen() {
    if (Status == Server_init) {
        // 启动接收和发送线程
        thread thread_recv(&RTP_Server::recv_thread, this);
        thread thread_send(&RTP_Server::send_thread, this);
        thread thread_loop(&RTP_Server::loop_thread, this);
        thread_recv.detach();
        thread_send.detach();
        thread_loop.detach();
    }
    // 服务器开始监听
    Status = Server_listen;
    return 0;
}

RTP_Client *RTP_Server::accept() {
    while (Status != Server_Close && Status != Server_Unknown_Error) {
        Status = Server_accept;
        {
            unique_lock<mutex> lock(client_mutex);
            for (auto it = clients_disconnected.begin(); it != clients_disconnected.end(); ++it) {
                if (it->second->Status == Client_Connected) {
                    auto client = it->second;
                    clients_connected[it->first] = it->second;
                    clients_disconnected.erase(it);
                    return client;
                }
            }
        }
        Sleep(sleep_time);
    }
    return nullptr;
}

void RTP_Server::close() {
    thread t1([&]() {
        for (auto [_, client]: clients_disconnected) {
            client->Status = Client_Connected_Close;
        }
    });
    // 已建立的连接 调用close
    thread t2([&]() {
        for (auto [_, client]: clients_connected) {
            client->close(5);
        }
        // 优雅关闭 3个线程都停止
        Status = Server_Close;
    });
    t1.join();
    t2.join();

    while (Status != Server_Close) {
        Sleep(sleep_time);
    }
    for (auto [_, client]: clients_disconnected) {
        delete client;
    }
    for (auto [_, client]: clients_connected) {
        delete client;
    }
}

// ================================================================================================================ //
// 生成校验和
static WORD check_sum_build(RTP_Datagram_t *RTP_Datagram) {
    WORD *word_array = (WORD *) RTP_Datagram;
    int times = sizeof(RTP_Datagram_t) / sizeof(WORD);
    DWORD temp = word_array[0];
    for (int i = 1; i < times; ++i) {
        temp += word_array[i];
        if (temp > 0xFFFF) {
            temp += (temp >> 16);
            temp &= 0xFFFF;
        }
    }
    return temp ^ 0xFFFF;
}

// 检测校验和
static bool check_sum_check(RTP_Datagram_t *RTP_Datagram) {
    return check_sum_build(RTP_Datagram) == 0;
}

// 生成标志
static RTP_Flag_t RTP_Flag_generate(BYTE ACK, BYTE SYN, BYTE FIN) {
    RTP_Flag_t flag{};
    flag.ACK = ACK;
    flag.SYN = SYN;
    flag.FIN = FIN;
    return flag;
}

// 生成报文头
static RTP_Head_t RTP_Head_generate(DWORD send_num, DWORD recv_num, DWORD windows_size, const RTP_Flag_t &flag) {
    RTP_Head_t head{};
    head.send_num = send_num;
    head.recv_num = recv_num;
    head.windows_num = windows_size;
    head.flag = flag;
    return head;
}

// 构造报文
static void RTP_Datagram_build(RTP_Datagram_t *RTP_Datagram, BYTE *buf, int len, const RTP_Head_t &head) {
    memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
    RTP_Datagram->head = head;
    RTP_Datagram->head.data_len = sizeof(RTP_Head_t) + len;
    memcpy(RTP_Datagram->data.data, buf, len);
    // 计算检验和
    RTP_Datagram->head.check_sum = check_sum_build(RTP_Datagram);
}

// 打印报文信息
static void print_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, const string &str, sockaddr_in *addr_ptr) {
#ifdef DEBUG_RTP
    unique_lock<mutex> lock(test_mutex);
    printf(
            "time: %lu %s addr: %d len: %lu data_len: %llu send_num: %lu recv_num: %lu ACK: %d SYN: %d FIN: %d\n",
            GetTickCount(),
            str.c_str(),
            addr_ptr->sin_port,
            RTP_Datagram->head.data_len,
            RTP_Datagram->head.data_len - sizeof(RTP_Head_t),
            RTP_Datagram->head.send_num,
            RTP_Datagram->head.recv_num,
            RTP_Datagram->head.flag.ACK,
            RTP_Datagram->head.flag.SYN,
            RTP_Datagram->head.flag.FIN
    );
#endif
}

// ================================================================================================================ //
// RTP_Client 收/发/重发线程
void RTP_Client::recv_thread() {
    // 申请 避免循环中重复申请
    auto RTP_Datagram = new RTP_Datagram_t();

    // 将数据存到接收缓冲区
    while (Status != Client_Connected_Close && Status != Client_Unknown_Error) {

        // 初始化状态等待 Connect
        if (Status == Client_init && Type == Common_Client) {
            Sleep(sleep_time);
            continue;
        }

        // 接收数据
        int len = recv_RTP_Datagram(RTP_Datagram);
        if (len == -1) {
            continue;
        }

        // 考虑是不是握手
        if (RTP_Datagram->head.flag.SYN == 1) {
            if (Type == Common_Client) {
                // 收到 SYN ACK
                if (RTP_Datagram->head.flag.ACK == 1) {
                    // 避免第三个握手包丢失后 重复初始化
                    if (Status == Client_Send_Shake_Hands_1) {
                        // 重设接收基址
                        recv_base = RTP_Datagram->head.send_num;
                        // 重设接收序号
                        recv_number = RTP_Datagram->head.send_num;
                        // 修改网络时延
                        network_delay = GetTickCount() - Shake_Hands_Send_Time + 100;
                        // 选择较小窗口
                        windows_number = RTP_Datagram->head.windows_num < windows_number ?
                                         RTP_Datagram->head.windows_num : windows_number;
                        // 重设缓冲大小
                        buffers_number = windows_number * 2 + 1;
                        recv_buffers.resize(buffers_number * DATA_LEN);
                        send_buffers.resize(buffers_number);
                        free_buffer_number = buffers_number;
                    }
                    // 状态转移
                    Status = Client_Send_Shake_Hands_3;
                }
            } else if (Type == Server_Client) {
                if (RTP_Datagram->head.flag.ACK == 0) {
                    // 避免第二个握手包丢失后 重复初始化
                    if (Status == Client_init) {
                        // 重设接收基址
                        recv_base = RTP_Datagram->head.send_num;
                        // 重设接收序号
                        recv_number = RTP_Datagram->head.send_num;
                        // 选择较小窗口
                        windows_number = RTP_Datagram->head.windows_num < windows_number ?
                                         RTP_Datagram->head.windows_num : windows_number;
                        // 重设缓冲区大小
                        buffers_number = windows_number * 2 + 1;
                        send_buffers.resize(buffers_number);
                        recv_buffers.resize(buffers_number * DATA_LEN);
                        free_buffer_number = buffers_number;
                    }
                    Status = Client_Send_Shake_Hands_2;
                    // 避免发送确认时 等一个网络延时
                    Shake_Hands_Send_Time = -1;
                } else if (RTP_Datagram->head.flag.ACK == 1) {
                    if (thread_resend == nullptr) {
                        thread_resend = new thread(&RTP_Client::resend_thread, this);
                        thread_resend->detach();
                    }
                    // 修改网络时延
                    network_delay = GetTickCount() - Shake_Hands_Send_Time + 100;
                    Status = Client_Connected;
                    isConnected = true;
                }
            } else {

            }
            continue;
        }

        // 考虑是不是挥手
        if (RTP_Datagram->head.flag.FIN == 1) {
            if (RTP_Datagram->head.flag.ACK == 0) {
                if (Status == Client_Send_Wave_Hands_1) {
                    // 收到第三个挥手报文
                    Status = Client_Send_Wave_Hands_4;
                } else {
                    // 收到第一个挥手报文
                    Status = Client_Send_Wave_Hands_23;
                }
            } else {
                if (Status == Client_Send_Wave_Hands_23) {
                    Status = Client_Connected_Close;
                    isConnected = false;
                }
            }
            continue;
        }

        // 考虑是不是确认数据
        if (RTP_Datagram->head.flag.ACK == 1) {
            // 确定不是多发的确认
            if (RTP_Datagram->head.recv_num > send_base) {
                // 修改发送窗口
                unique_lock<mutex> lock(send_mutex);
                for (auto &it: send_windows_list) {
                    if (it.send_number + send_buffers[it.index].first <= RTP_Datagram->head.recv_num) {
                        it.flag = true;
                    } else {
                        break;
                    }
                }
            }
            continue;
        }

        // 考虑是不是重传
        if (RTP_Datagram->head.send_num < recv_base + valid_buffer_index.begin()->second) {
            signals_queue.push(RTP_Flag_generate(1, 0, 0));
            continue;

        }

        // 数据拷贝
        {
            unique_lock<mutex> lock(recv_mutex);

            // 拷贝数据
            DWORD start = RTP_Datagram->head.send_num - recv_base;
            len -= sizeof(RTP_Head_t);

            // 数据异常
            if (start + len > recv_buffers.size()) {
                return;

            }
            copy(RTP_Datagram->data.data, RTP_Datagram->data.data + len, recv_buffers.begin() + start);

            // 更新索引
            bool flag = true;
            for (auto next = valid_buffer_index.begin(), item = next++;; ++item, ++next) {
                if (next == valid_buffer_index.end()) {
                    if (flag) {
                        if (item->first + item->second == start) {
                            item->second += len;
                        } else {
                            valid_buffer_index.emplace_back(start, len);
                        }
                    }
                    break;
                }

                if (item->first <= start && start <= next->first) {
                    flag = false;
                    if (item->first + item->second == start && start + len == next->first) {
                        item->second += len + next->second;
                        valid_buffer_index.erase(next);
                    } else if (item->first + item->second == start) {
                        item->second += len;
                    } else {
                        valid_buffer_index.insert(next, {start, len});
                    }
                    break;
                }
            }

            recv_number = recv_base + valid_buffer_index.begin()->second;

            // 确认
            signals_queue.push(RTP_Flag_generate(1, 0, 0));
        }
    }

    // 释放
    delete RTP_Datagram;
}

void RTP_Client::send_thread() {
    // 申请 避免循环中重复申请
    auto RTP_Datagram = new RTP_Datagram_t();

    while (Status != Client_Connected_Close && Status != Client_Unknown_Error) {

        // Send_FSM
        switch (Status) {
            case Client_create:
            case Client_init: {
                Sleep(sleep_time);
                continue;
            }

            case Client_Send_Shake_Hands_1: {
                // 发送握手 避免第一个包丢失
                if (GetTickCount() - Shake_Hands_Send_Time > network_delay) {
                    send_RTP_Signals(RTP_Datagram, RTP_Head_generate(send_number, recv_number, windows_number,
                                                                     RTP_Flag_generate(0, 1, 0)));
                    Shake_Hands_Send_Time = GetTickCount();
                }
                continue;
            }
            case Client_Send_Shake_Hands_2: {
                // 收到握手报文 响应确认
                if (GetTickCount() - Shake_Hands_Send_Time > network_delay) {
                    send_RTP_Signals(RTP_Datagram, RTP_Head_generate(send_number, recv_number, windows_number,
                                                                     RTP_Flag_generate(1, 1, 0)));
                    Shake_Hands_Send_Time = GetTickCount();
                }
                continue;
            }
            case Client_Send_Shake_Hands_3: {
                // 发送握手确认
                send_RTP_Signals(RTP_Datagram, RTP_Head_generate(send_number, recv_number, windows_number,
                                                                 RTP_Flag_generate(1, 1, 0)));
                Status = Client_Connected;
                isConnected = true;
                if (thread_resend == nullptr) {
                    thread_resend = new thread(&RTP_Client::resend_thread, this);
                    thread_resend->detach();
                }
                continue;
            }

            case Client_Send_Wave_Hands_1: {
                // 发送挥手第一帧
                send_RTP_Signals(RTP_Datagram, RTP_Head_generate(send_number, recv_number, windows_number,
                                                                 RTP_Flag_generate(0, 0, 1)));
                Sleep(network_delay);
                continue;
            }
            case Client_Send_Wave_Hands_23: {
                // 收到挥手报文 响应
                send_RTP_Signals(RTP_Datagram, RTP_Head_generate(send_number, recv_number, windows_number,
                                                                 RTP_Flag_generate(1, 0, 1)));
                send_RTP_Signals(RTP_Datagram, RTP_Head_generate(send_number, recv_number, windows_number,
                                                                 RTP_Flag_generate(0, 0, 1)));
                Sleep(network_delay);
                continue;
            }
            case Client_Send_Wave_Hands_4: {
                // 发送挥手确认
                send_RTP_Signals(RTP_Datagram, RTP_Head_generate(send_number, recv_number, windows_number,
                                                                 RTP_Flag_generate(1, 0, 1)));
                Status = Client_Wave_Hands_delay;
                Sleep(network_delay * 2);
                continue;
            }
            case Client_Wave_Hands_delay: {
                Status = Client_Connected_Close;
                isConnected = false;
                continue;
            }

            case Client_Connected:
                break;

            default:
                continue;
        }

        // 信号队列 (接收报文后的ACK及时响应)
        while (!signals_queue.empty()) {
            // 发送信号
            send_RTP_Signals(RTP_Datagram,
                             RTP_Head_generate(send_number, recv_number, windows_number, signals_queue.front()));
            signals_queue.pop();
        }

        // 无窗口或无数据则等待
        if (send_windows_list.size() == windows_number ||
            (valid_buffer_head == free_buffer_head && free_buffer_number != 0)) {
            Sleep(sleep_time);
            continue;
        }

        // 发送数据
        {
            unique_lock<mutex> lock(send_mutex);
            // 发送数据
            send_RTP_Datagram(RTP_Datagram,
                              RTP_Head_generate(send_number, recv_number, windows_number, RTP_Flag_generate(0, 0, 0)),
                              valid_buffer_head);

            // 添加窗口
            Attribute_t attribute{};
            attribute.time = GetTickCount();
            attribute.flag = false;
            attribute.index = valid_buffer_head;
            attribute.send_number = send_number;
            send_windows_list.push_back(attribute);

            // 调整索引
            send_number += send_buffers[valid_buffer_head].first;
            ++valid_buffer_head;
            valid_buffer_head %= buffers_number;

        }
    }

    // 释放
    delete RTP_Datagram;
}

void RTP_Client::resend_thread() {
    // 申请 避免循环中重复申请
    auto RTP_Datagram = new RTP_Datagram_t();

    while (Status != Client_Connected_Close && Status != Client_Unknown_Error) {
        auto it = send_windows_list.begin();
        while (it != send_windows_list.end()) {
            if (it->flag) {
                send_mutex.lock();
                // 基址改变
                send_base += send_buffers[it->index].first;
                // 更新索引
                ++free_buffer_number;
                // 清除头
                it = send_windows_list.erase(it);
                send_mutex.unlock();
                continue;
            } else {
                if (GetTickCount() - it->time > network_delay * 2) {
                    // 超时重传
                    send_RTP_Datagram(RTP_Datagram,
                                      RTP_Head_generate(it->send_number, recv_number, windows_number,
                                                        RTP_Flag_generate(0, 0, 0)),
                                      it->index);
                    it->time = GetTickCount();
                    break;
                }
            }
        }
        Sleep(sleep_time);
    }

    // 释放
    delete RTP_Datagram;
}

// ================================================================================================================ //
// RTP_Server 收/发/循环线程
void RTP_Server::recv_thread() {
    // 申请 避免循环中重复申请
    auto addr = new sockaddr_in();
    auto RTP_Datagram = new RTP_Datagram_t();

    // 接收数据并分发
    while (Status != Server_Close && Status != Server_Unknown_Error) {
        // 初始化状态等待 accept
        if (Status == Server_init || Status == Server_listen) {
            Sleep(sleep_time);
            continue;
        }

        // 接收数据
        memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
        udp_ptr->recv((char *) RTP_Datagram, sizeof(RTP_Datagram_t), *addr);
        // 做校验
        if (!check_sum_check(RTP_Datagram)) {
            continue;
        }

        // IP和端口的组合
        auto IP_PORT = ((UINT64) addr->sin_addr.S_un.S_addr << 16) + addr->sin_port;

        RTP_Client *client;
        if (clients_connected.find(IP_PORT) != clients_connected.end()) {
            client = clients_connected[IP_PORT];
        } else if (clients_disconnected.find(IP_PORT) != clients_disconnected.end()) {
            client = clients_disconnected[IP_PORT];
        } else {
            clients_disconnected[IP_PORT] = new RTP_Server_Client(this, addr);
            client = clients_disconnected[IP_PORT];
        }

        auto server_client = dynamic_cast<RTP_Server_Client *> (client);
        server_client->recv_queue.emplace(*RTP_Datagram);
    }

    delete addr;
    delete RTP_Datagram;
}

void RTP_Server::send_thread() {
    while (Status != Server_Close && Status != Server_Unknown_Error) {
        while (!send_queue.empty()) {
            auto [client, datagram] = send_queue.front();
            send_queue.pop();

            udp_ptr->send((char *) &datagram, (int) datagram.head.data_len, *client->addr_ptr);
        }
    }
}

void RTP_Server::loop_thread() {
    // TODO: 删除会有问题
//    while (Status != Server_Close && Status != Server_Unknown_Error) {
//        // 删除第一个有问题的客户端
//        for (auto it = clients_connected.begin(); it != clients_connected.end(); ++it) {
//            if (it->second->Status == Client_Connected_Close || it->second->Status == Client_Unknown_Error) {
//                delete it->second;
//                clients_connected.erase(it);
//                break;
//            }
//        }
//        Sleep(sleep_time);
//    }
}

// ================================================================================================================ //
// 多态部分
int RTP_Client::recv_RTP_Datagram(RTP_Datagram_t *RTP_Datagram) {
    // 接收数据
    memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));

    sockaddr_in addr{};
    int len = udp_ptr->recv((char *) RTP_Datagram, sizeof(RTP_Datagram_t), addr);

    // 判断是不是来自服务器的数据 (IP和端口的组合) 同时 判断校验和是否正确
    if (addr.sin_addr.S_un.S_addr != addr_ptr->sin_addr.S_un.S_addr
        || addr.sin_port != addr_ptr->sin_port
        || !check_sum_check(RTP_Datagram)) {
        return -1;
    }

    print_RTP_Datagram(RTP_Datagram, "recv RTP_Data: ", &addr);

    return len;
}

int RTP_Client::send_RTP_Signals(RTP_Datagram_t *RTP_Datagram, RTP_Head_t head) {
    RTP_Datagram_build(RTP_Datagram, nullptr, 0, head);

    int len = udp_ptr->send((char *) RTP_Datagram, (int) RTP_Datagram->head.data_len, *addr_ptr);

    print_RTP_Datagram(RTP_Datagram, "send RTP_Sign: ", addr_ptr);

    return len;
}

int RTP_Client::send_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, RTP_Head_t head, DWORD index) {
    RTP_Datagram_build(RTP_Datagram, send_buffers[index].second.data, send_buffers[index].first, head);

    int len = udp_ptr->send((char *) RTP_Datagram, (int) RTP_Datagram->head.data_len, *addr_ptr);

    print_RTP_Datagram(RTP_Datagram, "send RTP_Sign: ", addr_ptr);

    return len;
}

// ================================================================================================================ //
// RTP_Server::RTP_Server_Client 子类重写
int RTP_Server::RTP_Server_Client::recv_RTP_Datagram(RTP_Datagram_t *RTP_Datagram) {
    while (Status != Client_Connected_Close && Status != Client_Unknown_Error) {
        if (recv_queue.empty()) {
            Sleep(sleep_time);
            continue;
        }
        auto data = recv_queue.front();
        recv_queue.pop();
        memcpy(RTP_Datagram, &data, sizeof(RTP_Datagram_t));
        print_RTP_Datagram(RTP_Datagram, "recv RTP_Data: ", addr_ptr);
        return (int) RTP_Datagram->head.data_len;
    }
    return -1;
}

int RTP_Server::RTP_Server_Client::send_RTP_Signals(RTP_Datagram_t *RTP_Datagram, RTP_Head_t head) {
    RTP_Datagram_build(RTP_Datagram, nullptr, 0, head);

    this->server_ptr->send_queue.emplace(this, *RTP_Datagram);

    print_RTP_Datagram(RTP_Datagram, "send RTP_Sign: ", addr_ptr);

    return (int) RTP_Datagram->head.send_num;
}

int RTP_Server::RTP_Server_Client::send_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, RTP_Head_t head, DWORD index) {
    RTP_Datagram_build(RTP_Datagram, send_buffers[index].second.data, send_buffers[index].first, head);

    this->server_ptr->send_queue.emplace(this, *RTP_Datagram);

    print_RTP_Datagram(RTP_Datagram, "send RTP_Data: ", addr_ptr);

    return (int) RTP_Datagram->head.send_num;
}

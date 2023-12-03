#include "RTP.h"

// 生成校验和
static WORD check_sum_generate(RTP_Datagram_t *RTP_Datagram) {
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
    return check_sum_generate(RTP_Datagram) == 0;
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
static RTP_Head_t RTP_Head_generate(BYTE ACK, BYTE SYN, BYTE FIN,
                                    DWORD send_num, DWORD recv_num, DWORD windows_size) {
    RTP_Head_t head{};
    head.send_num = send_num;
    head.recv_num = recv_num;
    head.windows_num = windows_size;
    head.flag = RTP_Flag_generate(ACK, SYN, FIN);
    return head;
}

// 生成报文
static int RTP_Datagram_generate(RTP_Datagram_t *RTP_Datagram, BYTE *buf, int len,
                                 RTP_Flag_t flag, DWORD send_num, DWORD recv_num, DWORD windows_size) {
    memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
    RTP_Datagram->head.flag = flag;
    RTP_Datagram->head.send_num = send_num;
    RTP_Datagram->head.recv_num = recv_num;
    RTP_Datagram->head.windows_num = windows_size;
    memcpy(RTP_Datagram->data.data, buf, len);
    // 计算检验和
    RTP_Datagram->head.check_sum = check_sum_generate(RTP_Datagram);
    return (int) sizeof(RTP_Head_t) + len;
}

// RTP 提供给子类的接口
// 打印报文信息 用于测试
void RTP::print_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, int len, const string &str) {
#ifdef DEBUG_RTP
    if (len < 0) {
        return;
    }

    unique_lock<mutex> lock(test_mutex);
    cout << GetTickCount() << " " << str;
    printf(
            "len: %d data_len: %llu send_num: %lu recv_num: %lu ACK: %d SYN: %d FIN: %d\n",
            len,
            len - sizeof(RTP_Head_t),
            RTP_Datagram->head.send_num,
            RTP_Datagram->head.recv_num,
            RTP_Datagram->head.flag.ACK,
            RTP_Datagram->head.flag.SYN,
            RTP_Datagram->head.flag.FIN
    );
#endif
}

// 接收报文
int RTP::recv_RTP_Datagram(RTP_Datagram_t *RTP_Datagram) {
    // 接收数据
    memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
    int len = udp_ptr->recv((char *) RTP_Datagram, sizeof(RTP_Datagram_t), *addr_ptr);

    // 做校验
    if (!check_sum_check(RTP_Datagram)) {
        len = -1;
    }

    print_RTP_Datagram(RTP_Datagram, len, "recv RTP_Data ");

    return len;
}

// 发送 信号(建立连接, ACK SYN FIN)
int RTP::send_RTP_Signals(RTP_Datagram_t *RTP_Datagram, RTP_Head_t head) {
    int len = RTP_Datagram_generate(RTP_Datagram, nullptr, 0,
                                    head.flag, head.send_num, head.recv_num, head.windows_num);
    len = udp_ptr->send((char *) RTP_Datagram, len, *addr_ptr);

    print_RTP_Datagram(RTP_Datagram, len, "send RTP_Sign ");

    return len;
}

// 发送报文 从缓冲区
int RTP::send_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, RTP_Head_t head, DWORD index) {
    int len = RTP_Datagram_generate(RTP_Datagram,
                                    send_buffers[index].second.data, send_buffers[index].first,
                                    head.flag, head.send_num, head.recv_num, head.windows_num);
    len = udp_ptr->send((char *) RTP_Datagram, len, *addr_ptr);

    print_RTP_Datagram(RTP_Datagram, len, "send RTP_Data ");

    return len;
}

// 接收数据及缓冲区的处理
void RTP::handle_recv_Data(RTP_Datagram_t *RTP_Datagram, int len) {
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
        return;
    }

    // 考虑是不是重传
    if (RTP_Datagram->head.send_num < recv_base + valid_buffer_index.begin()->second) {
        signals_queue.push(RTP_Flag_generate(1, 0, 0));
        return;
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

// 发送数据及缓冲区的处理
void RTP::handle_send_Data(RTP_Datagram_t *RTP_Datagram) {
    // 信号队列 (接收报文后的ACK及时响应)
    while (!signals_queue.empty()) {
        // 获取信号
        RTP_Flag_t flag = signals_queue.front();
        // 发送信号
        send_RTP_Signals(RTP_Datagram,
                         RTP_Head_generate(flag.ACK, flag.SYN, flag.FIN, send_number, recv_number, windows_number));
        signals_queue.pop();
    }

    // 无窗口或无数据则等待
    if (send_windows_list.size() == windows_number
        || (valid_buffer_head == free_buffer_head && free_buffer_number != 0)) {
        Sleep(sleep_time);
        return;
    }

    // 发送数据
    {
        unique_lock<mutex> lock(send_mutex);
        // 发送数据
        send_RTP_Datagram(RTP_Datagram,
                          RTP_Head_generate(0, 0, 0, send_number, recv_number, windows_number),
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

// 重传数据及缓冲区的处理
void RTP::handle_resend_Data(RTP_Datagram_t *RTP_Datagram) {
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
                                  RTP_Head_generate(0, 0, 0, it->send_number, recv_number, windows_number),
                                  it->index);
                it->time = GetTickCount();
                break;
            }
        }
    }
    Sleep(sleep_time);
}

// RTP 提供给上层的接口
int RTP::recv(char *buf, int len, int timeout) {
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

int RTP::send(char *buf, int len, int timeout) {
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

// RTP_Server
void RTP_Server::recv_thread() {
    // 申请 避免循环中重复申请
    auto RTP_Datagram = new RTP_Datagram_t();

    // 将数据存到接收缓冲区
    while (Status != Server_Connected_Close && Status != Server_Unknown_Error) {

        // 初始化状态等待 accept
        if (Status == Server_init || Status == Server_listen) {
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
            if (RTP_Datagram->head.flag.ACK == 0) {
                // 避免第二个握手包丢失后 重复初始化
                if (Status == Server_accept) {
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
                Status = Server_Shake_Hands_2;
                // 避免发送确认时 等一个网络延时
                Shake_Hands_Send_Time = -1;
            } else if (RTP_Datagram->head.flag.ACK == 1) {
                if (Status != Server_Connected) {
                    thread_resend = new thread(&RTP_Server::resend_thread, this);
                    thread_resend->detach();
                }
                // 修改网络时延
                network_delay = GetTickCount() - Shake_Hands_Send_Time + 100;
                Status = Server_Connected;
                isConnected = true;
            } else {
                Status = Server_Unknown_Error;
            }
            continue;
        }

        // 考虑是不是挥手
        if (RTP_Datagram->head.flag.FIN == 1) {
            if (RTP_Datagram->head.flag.ACK == 0) {
                Status = Server_Wave_Hands_2;
            } else if (RTP_Datagram->head.flag.ACK == 1) {
                Status = Server_Connected_Close;
                isConnected = false;
            } else {
                Status = Server_Unknown_Error;
            }
            continue;
        }

        // 处理收到的数据
        handle_recv_Data(RTP_Datagram, len);
    }

    // 释放
    delete RTP_Datagram;
}

void RTP_Server::send_thread() {
    // 申请 避免循环中重复申请
    auto RTP_Datagram = new RTP_Datagram_t();

    while (Status != Server_Connected_Close && Status != Server_Unknown_Error) {

        // 先看当前状态
        switch (Status) {
            case Server_init:
            case Server_listen:
            case Server_accept: {
                Sleep(sleep_time);
                continue;
            }
            case Server_Shake_Hands_2: {
                // 收到握手报文 响应确认
                if (GetTickCount() - Shake_Hands_Send_Time > network_delay) {
                    send_RTP_Signals(RTP_Datagram,
                                     RTP_Head_generate(1, 1, 0, send_number, recv_number, windows_number));
                    Shake_Hands_Send_Time = GetTickCount();
                }
                continue;
            }
            case Server_Wave_Hands_2: {
                // 收到挥手报文 响应
                send_RTP_Signals(RTP_Datagram, RTP_Head_generate(1, 0, 1, send_number, recv_number, windows_number));
                send_RTP_Signals(RTP_Datagram, RTP_Head_generate(0, 0, 1, send_number, recv_number, windows_number));
                Sleep(network_delay);
                continue;
            }
            default:
                break;
        }

        // 处理发送的数据
        handle_send_Data(RTP_Datagram);
    }

    // 释放
    delete RTP_Datagram;
}

void RTP_Server::resend_thread() {
    // 申请 避免循环中重复申请
    auto RTP_Datagram = new RTP_Datagram_t();

    while (Status != Server_Connected_Close && Status != Server_Unknown_Error) {
        handle_resend_Data(RTP_Datagram);
    }

    // 释放
    delete RTP_Datagram;
}

// RTP_Client
void RTP_Client::recv_thread() {
    // 申请 避免循环中重复申请
    auto RTP_Datagram = new RTP_Datagram_t();

    // 将数据存到接收缓冲区
    while (Status != Client_Connected_Close && Status != Client_Unknown_Error) {

        // 初始化状态等待 Connect
        if (Status == Client_init) {
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
            // 收到 SYN ACK
            if (RTP_Datagram->head.flag.ACK == 1) {
                // 避免第三个握手包丢失后 重复初始化
                if (Status == Client_Shake_Hands_1) {
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
                Status = Client_Shake_Hands_3;
            } else {
                Status = Client_Unknown_Error;
            }
            continue;
        }

        // 考虑是不是挥手
        if (RTP_Datagram->head.flag.FIN == 1) {
            if (RTP_Datagram->head.flag.ACK == 1) {
            } else if (RTP_Datagram->head.flag.ACK == 0) {
                Status = Client_Wave_Hands_3;
            } else {
                Status = Client_Unknown_Error;
            }
            continue;
        }

        handle_recv_Data(RTP_Datagram, len);
    }

    // 释放
    delete RTP_Datagram;
}

void RTP_Client::send_thread() {
    // 申请 避免循环中重复申请
    auto RTP_Datagram = new RTP_Datagram_t();

    while (Status != Client_Connected_Close && Status != Client_Unknown_Error) {
        // 先看当前状态
        switch (Status) {
            case Client_init: {
                Sleep(sleep_time);
                continue;
            }
            case Client_Shake_Hands_1: {
                // 发送握手 避免第一个包丢失
                if (GetTickCount() - Shake_Hands_Send_Time > network_delay) {
                    send_RTP_Signals(RTP_Datagram,
                                     RTP_Head_generate(0, 1, 0, send_number, recv_number, windows_number));
                    Shake_Hands_Send_Time = GetTickCount();
                }
                continue;
            }
            case Client_Shake_Hands_3: {
                // 发送握手确认
                send_RTP_Signals(RTP_Datagram, RTP_Head_generate(1, 1, 0, send_number, recv_number, windows_number));
                Status = Client_Connected;
                isConnected = true;
                thread_resend = new thread(&RTP_Client::resend_thread, this);
                thread_resend->detach();
                continue;
            }
            case Client_Wave_Hands_1: {
                // 发送挥手
                send_RTP_Signals(RTP_Datagram, RTP_Head_generate(0, 0, 1, send_number, recv_number, windows_number));
                Sleep(network_delay);
                continue;
            }
            case Client_Wave_Hands_3: {
                // 发送挥手确认
                send_RTP_Signals(RTP_Datagram, RTP_Head_generate(1, 0, 1, send_number, recv_number, windows_number));
                Status = Client_Wave_Hands_5;
                Sleep(network_delay * 2);
                continue;
            }
            case Client_Wave_Hands_5: {
                Status = Client_Connected_Close;
                isConnected = false;
                continue;
            }
            default:
                break;
        }

        // 处理发送的数据
        handle_send_Data(RTP_Datagram);
    }

    // 释放
    delete RTP_Datagram;
}

void RTP_Client::resend_thread() {
    // 申请 避免循环中重复申请
    auto RTP_Datagram = new RTP_Datagram_t();

    while (Status != Client_Connected_Close && Status != Client_Unknown_Error) {
        handle_resend_Data(RTP_Datagram);
    }

    // 释放
    delete RTP_Datagram;
}
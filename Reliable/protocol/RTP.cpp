#include "RTP.h"
#include <memory>
WORD RTP::generate_sum_check(RTP_Datagram_t *RTP_Datagram) {
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

bool RTP::check_sum_check(RTP_Datagram_t *RTP_Datagram) {
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

int RTP::generate_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, char *buf, int len, BYTE ACK, BYTE SYN, BYTE FIN) const {
    memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
    RTP_Datagram->send_number = send_number;
    RTP_Datagram->recv_number = recv_number;
    RTP_Datagram->flag.ACK = ACK;
    RTP_Datagram->flag.SYN = SYN;
    RTP_Datagram->flag.FIN = FIN;
    RTP_Datagram->windows_size = windows_size;
    memcpy(RTP_Datagram->data, buf, len);
    // 计算检验和
    RTP_Datagram->sum_check = generate_sum_check(RTP_Datagram);
    return (int) sizeof(RTP_Datagram_t) - DATA_LEN + len;
}

int RTP::send_signals(RTP_Datagram_t *RTP_Datagram, byte ack, byte syn, byte fin) {
    // 回包
    int len = generate_RTP_Datagram(RTP_Datagram, temp_buf, 0, ack, syn, fin);
    len = udp_ptr->send((char *) RTP_Datagram, len, *addr_ptr);
    return len;
}

void RTP_Server::recv_thread() {
    // 申请 避免循环中重复申请
    auto *RTP_Datagram = new RTP_Datagram_t();

    // 将数据存到接收缓冲区
    while (Statu != Server_Connected_Close && Statu != Server_Unknown_Error) {

        // 初始化状态等待 accept
        if (Statu == Server_init || Statu == Server_listen) {
            Sleep(sleep_time);
            continue;
        }

        // 接收数据
        memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
        int len = udp_ptr->recv((char *) RTP_Datagram, sizeof(RTP_Datagram_t), *addr_ptr);

        // 做校验 检验出错直接丢包
        if (len == -1 || !check_sum_check(RTP_Datagram)) {
            continue;
        }

        // 考虑是不是握手
        if (RTP_Datagram->flag.SYN == 1) {
            if (RTP_Datagram->flag.ACK == 0) {
                // 避免第二个握手包丢失后 重复初始化
                if (Statu == Server_accept) {
                    // 选择较小窗口
                    windows_size = RTP_Datagram->windows_size < windows_size ?
                                   RTP_Datagram->windows_size : windows_size;
                    // 重设缓冲区大小
                    send_buf.resize(windows_size * DATA_LEN * 2);
                    recv_buf.resize(windows_size * DATA_LEN * 2);
                    // 重设接收序号
                    recv_number = RTP_Datagram->send_number;
                    // 重设接收基址
                    index_offset = RTP_Datagram->send_number;
                    // 初始化接收缓冲索引有效值
                    recv_buf_effective_index.emplace_back(0, 0);
                }
                Statu = Server_Shake_Hands_2;
                // 避免发送确认时 等一个网络延时
                Shake_Hands_Send_Time = -1;
            } else if (RTP_Datagram->flag.ACK == 1) {
                // 修改网络时延
                network_delay = GetTickCount() - Shake_Hands_Send_Time + 50;
                Statu = Server_Connected;
            } else {
                Statu = Server_Unknown_Error;
            }
            continue;
        }

        // 考虑是不是挥手
        if (RTP_Datagram->flag.FIN == 1) {
            if (RTP_Datagram->flag.ACK == 0) {
                Statu = Server_Wave_Hands_2;
            } else if (RTP_Datagram->flag.ACK == 1) {
                Statu = Server_Connected_Close;
            } else {
                Statu = Server_Unknown_Error;
            }
            continue;
        }

        // 考虑是不是重传 TODO： 重新设计一下
        if (RTP_Datagram->send_number < recv_number) {
            // 回包
//            reply_ack(1, 0, RTP_Datagram->flag.FIN);
            continue;
        }

        // 数据报文 考虑缓冲区满的情况
        // 数据域的长度
        unsigned data_len = len - (sizeof(RTP_Datagram_t) - DATA_LEN);
        // 获得缓冲区的起始位置
        unsigned start, end;
        while (true) {
            // 操作共享数据前加锁
            recv_mutex.lock();

            start = RTP_Datagram->send_number - index_offset;
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
        recv_number = index_offset + recv_buf_effective_index[0].second;
        // 操作共享数据后解锁
        recv_mutex.unlock();
        // 回包
//        reply_ack(1, 0, 0);
        cout << "接收 " << recv_number << " 成功" << endl;
    }

    // 释放
    delete RTP_Datagram;
}

void RTP_Server::send_thread() {
    // 申请 避免循环中重复申请
    auto *RTP_Datagram = new RTP_Datagram_t();

    while (Statu != Server_Connected_Close && Statu != Server_Unknown_Error) {

        // 先看当前状态
        switch (Statu) {
            case Server_init:
            case Server_accept: {
                Sleep(sleep_time);
                continue;
            }
            case Server_Shake_Hands_2: {
                // 收到握手报文 响应确认
                if (GetTickCount() - Shake_Hands_Send_Time > network_delay) {
                    send_signals(RTP_Datagram, 1, 1, 0);
                    Shake_Hands_Send_Time = GetTickCount();
                }
                continue;
            }
            case Server_Wave_Hands_2: {
                // 收到挥手报文 响应
                send_signals(RTP_Datagram, 1, 0, 1);
                send_signals(RTP_Datagram, 0, 0, 1);
                Sleep(network_delay);
                continue;
            }
            default:
                break;
        }

        // 在看信号队列 (接收报文后的ACK及时响应)
        while (!signals_queue.empty()) {
            // 获取信号
            Flag flag = signals_queue.front();
            // 发送信号
            int len = send_signals(RTP_Datagram, flag.ACK, flag.SYN, flag.FIN);
            if (len == -1) {
                Statu = Server_Unknown_Error;
                break;
            }
            signals_queue.pop();
        }

    }

    // 释放
    delete RTP_Datagram;
}

void RTP_Client::recv_thread() {
    // 申请 避免循环中重复申请
    auto *RTP_Datagram = new RTP_Datagram_t();

    // 将数据存到接收缓冲区
    while (Statu != Client_Connected_Close && Statu != Client_Unknown_Error) {

        // 初始化状态等待 Connect
        if (Statu == Client_init) {
            Sleep(sleep_time);
            continue;
        }

        // 接收数据
        memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
        int len = udp_ptr->recv((char *) RTP_Datagram, sizeof(RTP_Datagram_t), *addr_ptr);

        // 做校验 检验出错直接丢包
        if (len == -1 || !check_sum_check(RTP_Datagram)) {
            continue;
        }

        // 考虑是不是握手
        if (RTP_Datagram->flag.SYN == 1) {
            // 收到 SYN ACK
            if (RTP_Datagram->flag.ACK == 1) {
                // 避免第三个握手包丢失后 重复初始化
                if (Statu == Client_Shake_Hands_1) {
                    // 选择较小窗口
                    windows_size = RTP_Datagram->windows_size < windows_size ?
                                   RTP_Datagram->windows_size : windows_size;
                    // 修改网络时延
                    network_delay = GetTickCount() - Shake_Hands_Send_Time + 50;
                    // 重设缓冲区大小
                    send_buf.resize(windows_size * DATA_LEN * 2);
                    recv_buf.resize(windows_size * DATA_LEN * 2);
                    // 重设接收序号
                    recv_number = RTP_Datagram->send_number;
                    // 重设接收基址
                    index_offset = RTP_Datagram->send_number;
                    // 初始化接收缓冲索引有效值
                    recv_buf_effective_index.emplace_back(0, 0);
                }
                // 状态转移
                Statu = Client_Shake_Hands_3;
            } else {
                Statu = Client_Unknown_Error;
            }
            continue;
        }

        // 考虑是不是挥手
        if (RTP_Datagram->flag.FIN == 1) {
            if (RTP_Datagram->flag.ACK == 1) {
                // TODO: 挥手确认 啥也不做
            } else if (RTP_Datagram->flag.ACK == 0) {
                Statu = Client_Wave_Hands_3;
            } else {
                Statu = Client_Unknown_Error;
            }
            continue;
        }

        // 考虑是不是确认 TODO：调整窗口
        if (RTP_Datagram->flag.ACK == 1) {
            continue;
        }

//        // 将数据 放在接收缓冲区中 考虑缓冲区满的情况
//        // 数据域的长度
//        unsigned data_len = len - (sizeof(RTP_Datagram_t) - DATA_LEN);
//        // 获得缓冲区的起始位置
//        unsigned start, end;
//        while (true) {
//            // 操作共享数据前加锁
//            recv_mutex.lock();
//
//            start = RTP_Datagram->send_number - index_offset;
//            end = start + data_len;
//
//            // 缓冲区可以放下就跳出
//            if (end <= recv_buf.size()) {
//                break;
//            }
//            // 缓冲区放不下就开锁, 等待上层调用
//            recv_mutex.unlock();
//            Sleep(sleep_time);
//        }
//        // 拷贝数据域到缓冲区
//        copy(RTP_Datagram->data, RTP_Datagram->data + data_len, recv_buf.begin() + start);
//        // 将新的片段插入 recv_buf_effective_index 中
//        if (recv_buf_effective_index.size() == 1) {
//            recv_buf_effective_index.emplace_back(pair<unsigned, unsigned>(start, end));
//        } else {
//            for (int i = 1; i < recv_buf_effective_index.size(); i++) {
//                if (start >= recv_buf_effective_index[i - 1].second
//                    && end <= recv_buf_effective_index[i].first) {
//                    recv_buf_effective_index.insert(
//                            recv_buf_effective_index.begin() + i,
//                            pair<unsigned, unsigned>(start, end)
//                    );
//                    break;
//                }
//            }
//        }
//        // 合并 recv_buf_effective_index 中重复的索引
//        for (int i = 1; i < recv_buf_effective_index.size(); i++) {
//            if (recv_buf_effective_index[i].first <= recv_buf_effective_index[i - 1].second) {
//                recv_buf_effective_index[i - 1].second = recv_buf_effective_index[i].second;
//                recv_buf_effective_index.erase(recv_buf_effective_index.begin() + i);
//                --i;
//            }
//        }
//        // 接收数调整
//        recv_number = index_offset + recv_buf_effective_index[0].second;
//        // 操作共享数据后解锁
//        recv_mutex.unlock();
//        // 回包
////        reply_ack(1, 0, 0);
//        cout << "接收 " << recv_number << " 成功" << endl;
    }

    // 释放
    delete RTP_Datagram;
}

void RTP_Client::send_thread() {
    // 申请 避免循环中重复申请
    auto RTP_Datagram = new RTP_Datagram_t();

    // 将发送缓冲区的数据发送出去
    while (Statu != Client_Connected_Close && Statu != Client_Unknown_Error) {

        // 先看当前状态
        switch (Statu) {
            case Client_init: {
                Sleep(sleep_time);
                continue;
            }
            case Client_Shake_Hands_1: {
                // 发送握手 避免第一个包丢失
                if (GetTickCount() - Shake_Hands_Send_Time > network_delay) {
                    send_signals(RTP_Datagram, 0, 1, 0);
                    Shake_Hands_Send_Time = GetTickCount();
                }
                continue;
            }
            case Client_Shake_Hands_3: {
                // 发送握手确认
                send_signals(RTP_Datagram, 1, 1, 0);
                Statu = Client_Connected;
                continue;
            }
            case Client_Wave_Hands_1: {
                // 发送挥手
                send_signals(RTP_Datagram, 0, 0, 1);
                Sleep(network_delay);
                continue;
            }
            case Client_Wave_Hands_3: {
                // 发送挥手确认
                send_signals(RTP_Datagram, 1, 0, 1);
                Statu = Client_Wave_Hands_5;
                Sleep(network_delay * 2);
                continue;
            }
            case Client_Wave_Hands_5: {
                Statu = Client_Connected_Close;
                continue;
            }
            default:
                break;
        }

        // 在看信号队列 TODO: 现在应该不会有信号 因为是单向传递
        while (!signals_queue.empty()) {
            // 获取信号
            Flag flag = signals_queue.front();
            // 发送信号
            int len = send_signals(RTP_Datagram, flag.ACK, flag.SYN, flag.FIN);
            if (len == -1) {
                Statu = Client_Unknown_Error;
                break;
            }
            signals_queue.pop();
        }

//        int send_len = (send_buf_effective_index < DATA_LEN) ? (int) send_buf_effective_index : DATA_LEN;
//
//        if (send_len == 0) {
//            Sleep(sleep_time);
//            continue;
//        }
//
//        // 操作共享数据前加锁
//        send_mutex.lock();
//
//        // 拷贝数据并发送
//        copy(send_buf.begin(), send_buf.begin() + send_len, temp_buf);
//
//        int len;
//        len = generate_RTP_Datagram(RTP_Datagram, temp_buf, send_len, 0, 0, 0);
//        len = udp_ptr->send((char *) RTP_Datagram, len, *addr_ptr);

//        // 启动 定时器
//        bool ack_flag = false;
//        thread timer(&RTP_Client::send_again_thread, this, RTP_Datagram_send, len, ref(ack_flag));
//        timer.detach();
//
//        auto *RTP_Datagram = new RTP_Datagram_t();
//        memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
//        len = udp_ptr->recv((char *) RTP_Datagram, sizeof(RTP_Datagram_t), *addr_ptr);
//
//        // 收到ACK响应包 清空 RTP_Datagram_send
//        ack_flag = true;
//        delete RTP_Datagram_send;
//
//        if (len == -1 || !check_sum_check(RTP_Datagram)
//            || RTP_Datagram->flag.ACK != 1
//            || RTP_Datagram->recv_number != send_number + send_len) {
//            delete RTP_Datagram;
//            // 操作共享数据后解锁
//            send_mutex.unlock();
//            continue;
//        } else {
//            // 调整收发数 以及 缓冲区
//            send_number += send_len;
//            copy(send_buf.begin() + send_len, send_buf.end(), send_buf.begin());
//            send_buf_effective_index -= send_len;
//
//            delete RTP_Datagram;
//            // 操作共享数据后解锁
//            send_mutex.unlock();
//        }
//        cout << "发送" << send_number << " 成功" << endl;
    }

    // 释放
    delete RTP_Datagram;
}

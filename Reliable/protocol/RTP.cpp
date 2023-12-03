#include "RTP.h"

// ����У���
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

// ���У���
static bool check_sum_check(RTP_Datagram_t *RTP_Datagram) {
    return check_sum_generate(RTP_Datagram) == 0;
}

// ���ɱ�־
static RTP_Flag_t RTP_Flag_generate(BYTE ACK, BYTE SYN, BYTE FIN) {
    RTP_Flag_t flag{};
    flag.ACK = ACK;
    flag.SYN = SYN;
    flag.FIN = FIN;
    return flag;
}

// ���ɱ���ͷ
static RTP_Head_t RTP_Head_generate(BYTE ACK, BYTE SYN, BYTE FIN,
                                    DWORD send_num, DWORD recv_num, DWORD windows_size) {
    RTP_Head_t head{};
    head.send_num = send_num;
    head.recv_num = recv_num;
    head.windows_num = windows_size;
    head.flag = RTP_Flag_generate(ACK, SYN, FIN);
    return head;
}

// ���ɱ���
static int RTP_Datagram_generate(RTP_Datagram_t *RTP_Datagram, BYTE *buf, int len,
                                 RTP_Flag_t flag, DWORD send_num, DWORD recv_num, DWORD windows_size) {
    memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
    RTP_Datagram->head.flag = flag;
    RTP_Datagram->head.send_num = send_num;
    RTP_Datagram->head.recv_num = recv_num;
    RTP_Datagram->head.windows_num = windows_size;
    memcpy(RTP_Datagram->data.data, buf, len);
    // ��������
    RTP_Datagram->head.check_sum = check_sum_generate(RTP_Datagram);
    return (int) sizeof(RTP_Head_t) + len;
}

// RTP �ṩ������Ľӿ�
// ��ӡ������Ϣ ���ڲ���
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

// ���ձ���
int RTP::recv_RTP_Datagram(RTP_Datagram_t *RTP_Datagram) {
    // ��������
    memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
    int len = udp_ptr->recv((char *) RTP_Datagram, sizeof(RTP_Datagram_t), *addr_ptr);

    // ��У��
    if (!check_sum_check(RTP_Datagram)) {
        len = -1;
    }

    print_RTP_Datagram(RTP_Datagram, len, "recv RTP_Data ");

    return len;
}

// ���� �ź�(��������, ACK SYN FIN)
int RTP::send_RTP_Signals(RTP_Datagram_t *RTP_Datagram, RTP_Head_t head) {
    int len = RTP_Datagram_generate(RTP_Datagram, nullptr, 0,
                                    head.flag, head.send_num, head.recv_num, head.windows_num);
    len = udp_ptr->send((char *) RTP_Datagram, len, *addr_ptr);

    print_RTP_Datagram(RTP_Datagram, len, "send RTP_Sign ");

    return len;
}

// ���ͱ��� �ӻ�����
int RTP::send_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, RTP_Head_t head, DWORD index) {
    int len = RTP_Datagram_generate(RTP_Datagram,
                                    send_buffers[index].second.data, send_buffers[index].first,
                                    head.flag, head.send_num, head.recv_num, head.windows_num);
    len = udp_ptr->send((char *) RTP_Datagram, len, *addr_ptr);

    print_RTP_Datagram(RTP_Datagram, len, "send RTP_Data ");

    return len;
}

// �������ݼ��������Ĵ���
void RTP::handle_recv_Data(RTP_Datagram_t *RTP_Datagram, int len) {
    // �����ǲ���ȷ������
    if (RTP_Datagram->head.flag.ACK == 1) {
        // ȷ�����Ƕ෢��ȷ��
        if (RTP_Datagram->head.recv_num > send_base) {
            // �޸ķ��ʹ���
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

    // �����ǲ����ش�
    if (RTP_Datagram->head.send_num < recv_base + valid_buffer_index.begin()->second) {
        signals_queue.push(RTP_Flag_generate(1, 0, 0));
        return;
    }

    // ���ݿ���
    {
        unique_lock<mutex> lock(recv_mutex);

        // ��������
        DWORD start = RTP_Datagram->head.send_num - recv_base;
        len -= sizeof(RTP_Head_t);

        // �����쳣
        if (start + len > recv_buffers.size()) {
            return;

        }
        copy(RTP_Datagram->data.data, RTP_Datagram->data.data + len, recv_buffers.begin() + start);

        // ��������
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

        // ȷ��
        signals_queue.push(RTP_Flag_generate(1, 0, 0));
    }
}

// �������ݼ��������Ĵ���
void RTP::handle_send_Data(RTP_Datagram_t *RTP_Datagram) {
    // �źŶ��� (���ձ��ĺ��ACK��ʱ��Ӧ)
    while (!signals_queue.empty()) {
        // ��ȡ�ź�
        RTP_Flag_t flag = signals_queue.front();
        // �����ź�
        send_RTP_Signals(RTP_Datagram,
                         RTP_Head_generate(flag.ACK, flag.SYN, flag.FIN, send_number, recv_number, windows_number));
        signals_queue.pop();
    }

    // �޴��ڻ���������ȴ�
    if (send_windows_list.size() == windows_number
        || (valid_buffer_head == free_buffer_head && free_buffer_number != 0)) {
        Sleep(sleep_time);
        return;
    }

    // ��������
    {
        unique_lock<mutex> lock(send_mutex);
        // ��������
        send_RTP_Datagram(RTP_Datagram,
                          RTP_Head_generate(0, 0, 0, send_number, recv_number, windows_number),
                          valid_buffer_head);

        // ��Ӵ���
        Attribute_t attribute{};
        attribute.time = GetTickCount();
        attribute.flag = false;
        attribute.index = valid_buffer_head;
        attribute.send_number = send_number;
        send_windows_list.push_back(attribute);

        // ��������
        send_number += send_buffers[valid_buffer_head].first;
        ++valid_buffer_head;
        valid_buffer_head %= buffers_number;

    }
}

// �ش����ݼ��������Ĵ���
void RTP::handle_resend_Data(RTP_Datagram_t *RTP_Datagram) {
    auto it = send_windows_list.begin();
    while (it != send_windows_list.end()) {
        if (it->flag) {
            send_mutex.lock();
            // ��ַ�ı�
            send_base += send_buffers[it->index].first;
            // ��������
            ++free_buffer_number;
            // ���ͷ
            it = send_windows_list.erase(it);
            send_mutex.unlock();
            continue;
        } else {
            if (GetTickCount() - it->time > network_delay * 2) {
                // ��ʱ�ش�
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

// RTP �ṩ���ϲ�Ľӿ�
int RTP::recv(char *buf, int len, int timeout) {
    DWORD start = GetTickCount();
    while (true) {
        // ����ʱ
        if (!isConnected || GetTickCount() - start >= timeout) {
            return -1;
        }

        // ������
        if (valid_buffer_index.begin()->second == 0) {
            Sleep(sleep_time);
            continue;
        }

        // �����ݼ���
        int length;
        {
            unique_lock<mutex> lock(recv_mutex);

            // ������
            length = min(len, valid_buffer_index.begin()->second);
            copy(recv_buffers.begin(), recv_buffers.begin() + length, buf);
            copy(recv_buffers.begin() + length, recv_buffers.end(), recv_buffers.begin());

            // ��������
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
        // д��ʱ
        if (!isConnected || GetTickCount() - start >= timeout) {
            return -1;
        }

        // ����������
        if (free_buffer_number == 0) {
            Sleep(sleep_time);
            continue;
        }

        // ���ܷ�����
        {
            unique_lock<mutex> lock(send_mutex);
            // ��������
            int length = len < DATA_LEN ? len : DATA_LEN;
            send_buffers[free_buffer_head].first = length;
            memcpy(send_buffers[free_buffer_head].second.data, buf + offset, length);

            // ��������
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
    // ���� ����ѭ�����ظ�����
    auto RTP_Datagram = new RTP_Datagram_t();

    // �����ݴ浽���ջ�����
    while (Status != Server_Connected_Close && Status != Server_Unknown_Error) {

        // ��ʼ��״̬�ȴ� accept
        if (Status == Server_init || Status == Server_listen) {
            Sleep(sleep_time);
            continue;
        }

        // ��������
        int len = recv_RTP_Datagram(RTP_Datagram);
        if (len == -1) {
            continue;
        }

        // �����ǲ�������
        if (RTP_Datagram->head.flag.SYN == 1) {
            if (RTP_Datagram->head.flag.ACK == 0) {
                // ����ڶ������ְ���ʧ�� �ظ���ʼ��
                if (Status == Server_accept) {
                    // ������ջ�ַ
                    recv_base = RTP_Datagram->head.send_num;
                    // ����������
                    recv_number = RTP_Datagram->head.send_num;
                    // ѡ���С����
                    windows_number = RTP_Datagram->head.windows_num < windows_number ?
                                     RTP_Datagram->head.windows_num : windows_number;
                    // ���軺������С
                    buffers_number = windows_number * 2 + 1;
                    send_buffers.resize(buffers_number);
                    recv_buffers.resize(buffers_number * DATA_LEN);
                    free_buffer_number = buffers_number;
                }
                Status = Server_Shake_Hands_2;
                // ���ⷢ��ȷ��ʱ ��һ��������ʱ
                Shake_Hands_Send_Time = -1;
            } else if (RTP_Datagram->head.flag.ACK == 1) {
                if (Status != Server_Connected) {
                    thread_resend = new thread(&RTP_Server::resend_thread, this);
                    thread_resend->detach();
                }
                // �޸�����ʱ��
                network_delay = GetTickCount() - Shake_Hands_Send_Time + 100;
                Status = Server_Connected;
                isConnected = true;
            } else {
                Status = Server_Unknown_Error;
            }
            continue;
        }

        // �����ǲ��ǻ���
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

        // �����յ�������
        handle_recv_Data(RTP_Datagram, len);
    }

    // �ͷ�
    delete RTP_Datagram;
}

void RTP_Server::send_thread() {
    // ���� ����ѭ�����ظ�����
    auto RTP_Datagram = new RTP_Datagram_t();

    while (Status != Server_Connected_Close && Status != Server_Unknown_Error) {

        // �ȿ���ǰ״̬
        switch (Status) {
            case Server_init:
            case Server_listen:
            case Server_accept: {
                Sleep(sleep_time);
                continue;
            }
            case Server_Shake_Hands_2: {
                // �յ����ֱ��� ��Ӧȷ��
                if (GetTickCount() - Shake_Hands_Send_Time > network_delay) {
                    send_RTP_Signals(RTP_Datagram,
                                     RTP_Head_generate(1, 1, 0, send_number, recv_number, windows_number));
                    Shake_Hands_Send_Time = GetTickCount();
                }
                continue;
            }
            case Server_Wave_Hands_2: {
                // �յ����ֱ��� ��Ӧ
                send_RTP_Signals(RTP_Datagram, RTP_Head_generate(1, 0, 1, send_number, recv_number, windows_number));
                send_RTP_Signals(RTP_Datagram, RTP_Head_generate(0, 0, 1, send_number, recv_number, windows_number));
                Sleep(network_delay);
                continue;
            }
            default:
                break;
        }

        // �����͵�����
        handle_send_Data(RTP_Datagram);
    }

    // �ͷ�
    delete RTP_Datagram;
}

void RTP_Server::resend_thread() {
    // ���� ����ѭ�����ظ�����
    auto RTP_Datagram = new RTP_Datagram_t();

    while (Status != Server_Connected_Close && Status != Server_Unknown_Error) {
        handle_resend_Data(RTP_Datagram);
    }

    // �ͷ�
    delete RTP_Datagram;
}

// RTP_Client
void RTP_Client::recv_thread() {
    // ���� ����ѭ�����ظ�����
    auto RTP_Datagram = new RTP_Datagram_t();

    // �����ݴ浽���ջ�����
    while (Status != Client_Connected_Close && Status != Client_Unknown_Error) {

        // ��ʼ��״̬�ȴ� Connect
        if (Status == Client_init) {
            Sleep(sleep_time);
            continue;
        }

        // ��������
        int len = recv_RTP_Datagram(RTP_Datagram);
        if (len == -1) {
            continue;
        }

        // �����ǲ�������
        if (RTP_Datagram->head.flag.SYN == 1) {
            // �յ� SYN ACK
            if (RTP_Datagram->head.flag.ACK == 1) {
                // ������������ְ���ʧ�� �ظ���ʼ��
                if (Status == Client_Shake_Hands_1) {
                    // ������ջ�ַ
                    recv_base = RTP_Datagram->head.send_num;
                    // ����������
                    recv_number = RTP_Datagram->head.send_num;
                    // �޸�����ʱ��
                    network_delay = GetTickCount() - Shake_Hands_Send_Time + 100;
                    // ѡ���С����
                    windows_number = RTP_Datagram->head.windows_num < windows_number ?
                                     RTP_Datagram->head.windows_num : windows_number;
                    // ���軺���С
                    buffers_number = windows_number * 2 + 1;
                    recv_buffers.resize(buffers_number * DATA_LEN);
                    send_buffers.resize(buffers_number);
                    free_buffer_number = buffers_number;
                }
                // ״̬ת��
                Status = Client_Shake_Hands_3;
            } else {
                Status = Client_Unknown_Error;
            }
            continue;
        }

        // �����ǲ��ǻ���
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

    // �ͷ�
    delete RTP_Datagram;
}

void RTP_Client::send_thread() {
    // ���� ����ѭ�����ظ�����
    auto RTP_Datagram = new RTP_Datagram_t();

    while (Status != Client_Connected_Close && Status != Client_Unknown_Error) {
        // �ȿ���ǰ״̬
        switch (Status) {
            case Client_init: {
                Sleep(sleep_time);
                continue;
            }
            case Client_Shake_Hands_1: {
                // �������� �����һ������ʧ
                if (GetTickCount() - Shake_Hands_Send_Time > network_delay) {
                    send_RTP_Signals(RTP_Datagram,
                                     RTP_Head_generate(0, 1, 0, send_number, recv_number, windows_number));
                    Shake_Hands_Send_Time = GetTickCount();
                }
                continue;
            }
            case Client_Shake_Hands_3: {
                // ��������ȷ��
                send_RTP_Signals(RTP_Datagram, RTP_Head_generate(1, 1, 0, send_number, recv_number, windows_number));
                Status = Client_Connected;
                isConnected = true;
                thread_resend = new thread(&RTP_Client::resend_thread, this);
                thread_resend->detach();
                continue;
            }
            case Client_Wave_Hands_1: {
                // ���ͻ���
                send_RTP_Signals(RTP_Datagram, RTP_Head_generate(0, 0, 1, send_number, recv_number, windows_number));
                Sleep(network_delay);
                continue;
            }
            case Client_Wave_Hands_3: {
                // ���ͻ���ȷ��
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

        // �����͵�����
        handle_send_Data(RTP_Datagram);
    }

    // �ͷ�
    delete RTP_Datagram;
}

void RTP_Client::resend_thread() {
    // ���� ����ѭ�����ظ�����
    auto RTP_Datagram = new RTP_Datagram_t();

    while (Status != Client_Connected_Close && Status != Client_Unknown_Error) {
        handle_resend_Data(RTP_Datagram);
    }

    // �ͷ�
    delete RTP_Datagram;
}
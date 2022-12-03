#include "RTP.h"

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

int RTP::generate_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, char *buf, int len,
                               BYTE ACK, BYTE SYN, BYTE FIN, DWORD number) const {
    memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
    RTP_Datagram->send_number = number;
    RTP_Datagram->recv_number = recv_number;
    RTP_Datagram->flag.ACK = ACK;
    RTP_Datagram->flag.SYN = SYN;
    RTP_Datagram->flag.FIN = FIN;
    RTP_Datagram->windows_size = windows_size;
    memcpy(RTP_Datagram->data, buf, len);
    // ��������
    RTP_Datagram->sum_check = generate_sum_check(RTP_Datagram);
    return (int) sizeof(RTP_Datagram_t) - DATA_LEN + len;
}

int RTP::send_signals(RTP_Datagram_t *RTP_Datagram, byte ack, byte syn, byte fin) {
    int len = generate_RTP_Datagram(RTP_Datagram, temp_buf, 0, ack, syn, fin, send_number);
    len = udp_ptr->send((char *) RTP_Datagram, len, *addr_ptr);
    return len;
}

int RTP::send_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, DWORD number, int begin, int length) {
    int buf_end_2_begin = send_buf_size - begin;
    if (buf_end_2_begin >= length) {
        copy(send_buf.begin() + begin, send_buf.begin() + begin + length, temp_buf);
    } else {
        copy(send_buf.begin() + begin, send_buf.end(), temp_buf);
        copy(send_buf.begin(), send_buf.begin() + length - buf_end_2_begin, temp_buf + buf_end_2_begin);
    }
    int len;
    len = generate_RTP_Datagram(RTP_Datagram, temp_buf, length, 0, 0, 0, number);
    len = udp_ptr->send((char *) RTP_Datagram, len, *addr_ptr);
    cout << "send: " << number << endl;

    return len;
}

void RTP_Server::recv_thread() {
    // ���� ����ѭ�����ظ�����
    auto RTP_Datagram = new RTP_Datagram_t();

    // �����ݴ浽���ջ�����
    while (Statu != Server_Connected_Close && Statu != Server_Unknown_Error) {

        // ��ʼ��״̬�ȴ� accept
        if (Statu == Server_init || Statu == Server_listen) {
            Sleep(sleep_time);
            continue;
        }

        // ��������
        memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
        int len = udp_ptr->recv((char *) RTP_Datagram, sizeof(RTP_Datagram_t), *addr_ptr);

        // ��У�� �������ֱ�Ӷ���
        if (len == -1 || !check_sum_check(RTP_Datagram)) {
            continue;
        }

        // �����ǲ�������
        if (RTP_Datagram->flag.SYN == 1) {
            if (RTP_Datagram->flag.ACK == 0) {
                // ����ڶ������ְ���ʧ�� �ظ���ʼ��
                if (Statu == Server_accept) {
                    // ѡ���С����
                    windows_size = RTP_Datagram->windows_size < windows_size ?
                                   RTP_Datagram->windows_size : windows_size;
                    // ���軺������С
                    send_buf_size = windows_size * DATA_LEN * 2;
                    recv_buf_size = windows_size * DATA_LEN * 2;
                    send_buf.resize(send_buf_size);
                    recv_buf.resize(recv_buf_size);

                    // ����������
                    recv_number = RTP_Datagram->send_number;
                    // ������ջ�ַ
                    base_index = RTP_Datagram->send_number;
                    // ��ʼ������������Чֵ
                    recv_buf_effective_index.emplace_back(0, 0);
                }
                Statu = Server_Shake_Hands_2;
                // ���ⷢ��ȷ��ʱ ��һ��������ʱ
                Shake_Hands_Send_Time = -1;
            } else if (RTP_Datagram->flag.ACK == 1) {
                // �޸�����ʱ��
                network_delay = GetTickCount() - Shake_Hands_Send_Time + 100;
                Statu = Server_Connected;
                isConnected = true;
            } else {
                Statu = Server_Unknown_Error;
            }
            continue;
        }

        // �����ǲ��ǻ���
        if (RTP_Datagram->flag.FIN == 1) {
            if (RTP_Datagram->flag.ACK == 0) {
                Statu = Server_Wave_Hands_2;
            } else if (RTP_Datagram->flag.ACK == 1) {
                Statu = Server_Connected_Close;
                isConnected = false;
            } else {
                Statu = Server_Unknown_Error;
            }
            continue;
        }

        // �����ǲ����ش�
        if (RTP_Datagram->send_number < base_index) {
            Flag flag{};
            flag.ACK = 1;
            signals_queue.push(flag);
            continue;
        }

        // ���ݱ��� ���ǻ������������
        // ������ĳ���
        unsigned data_len = len - (sizeof(RTP_Datagram_t) - DATA_LEN);
        // ��û���������ʼλ��
        unsigned start, end;
        while (true) {
            // ������������ǰ����
            recv_mutex.lock();

            start = RTP_Datagram->send_number - base_index;
            end = start + data_len;

            // ���������Է��¾�����
            if (end <= recv_buf_size) {
                break;
            }
            // �������Ų��¾Ϳ���, �ȴ��ϲ����
            recv_mutex.unlock();
            Sleep(sleep_time);
        }
        // ���������򵽻�����
        copy(RTP_Datagram->data, RTP_Datagram->data + data_len, recv_buf.begin() + start);
        // ���µ�Ƭ�β��� recv_buf_effective_index ��
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
        // �ϲ� recv_buf_effective_index ���ظ�������
        for (int i = 1; i < recv_buf_effective_index.size(); i++) {
            if (recv_buf_effective_index[i].first <= recv_buf_effective_index[i - 1].second) {
                recv_buf_effective_index[i - 1].second = recv_buf_effective_index[i].second;
                recv_buf_effective_index.erase(recv_buf_effective_index.begin() + i);
                --i;
            }
        }
        // ����������
        recv_number = base_index + recv_buf_effective_index[0].second;
        cout << "recv: " << recv_number << endl;
        // �����������ݺ����
        recv_mutex.unlock();
        // �ذ�
        Flag flag{};
        flag.ACK = 1;
        signals_queue.push(flag);
    }

    // �ͷ�
    delete RTP_Datagram;
}

void RTP_Server::send_thread() {
    // ���� ����ѭ�����ظ�����
    auto RTP_Datagram = new RTP_Datagram_t();

    while (Statu != Server_Connected_Close && Statu != Server_Unknown_Error) {

        // �ȿ���ǰ״̬
        switch (Statu) {
            case Server_init:
            case Server_accept: {
                Sleep(sleep_time);
                continue;
            }
            case Server_Shake_Hands_2: {
                // �յ����ֱ��� ��Ӧȷ��
                if (GetTickCount() - Shake_Hands_Send_Time > network_delay) {
                    send_signals(RTP_Datagram, 1, 1, 0);
                    Shake_Hands_Send_Time = GetTickCount();
                }
                continue;
            }
            case Server_Wave_Hands_2: {
                // �յ����ֱ��� ��Ӧ
                send_signals(RTP_Datagram, 1, 0, 1);
                send_signals(RTP_Datagram, 0, 0, 1);
                Sleep(network_delay);
                continue;
            }
            default:
                break;
        }

        // ����ֻ̬����һ����
        while (Statu == Server_Connected) {
            // �ڿ��źŶ��� (���ձ��ĺ��ACK��ʱ��Ӧ)
            while (!signals_queue.empty()) {
                // ��ȡ�ź�
                Flag flag = signals_queue.front();
                // �����ź�
                int len = send_signals(RTP_Datagram, flag.ACK, flag.SYN, flag.FIN);
                if (len == -1) {
                    Statu = Server_Unknown_Error;
                    break;
                }
                signals_queue.pop();
            }
        }
    }

    // �ͷ�
    delete RTP_Datagram;
}

void RTP_Client::resend_thread() {
    // ���� ����ѭ�����ظ�����
    auto RTP_Datagram = new RTP_Datagram_t();

    while (Statu != Client_Connected_Close && Statu != Client_Unknown_Error) {
        send_window_mutex.lock();
        auto it = send_window.begin();
        while (it != send_window.end()) {
            if (it->flag && it == send_window.begin()) {
                // ��ַ�ı�
                add_index_offset(it->length);
                // ��Ч������ʼ�ı�
                set_send_buf_effective_index_first(it->begin, it->length);
                // ���ͷ
                it = send_window.erase(it);
                continue;
            }

            if (!it->flag && GetTickCount() - it->time > network_delay * 2) {
                // ��ʱ�ش�
                send_mutex.lock();
                cout << "resend ";
                send_RTP_Datagram(RTP_Datagram, it->send_number, it->begin, it->length);
                it->time = GetTickCount();
                send_mutex.unlock();
            }

            ++it;
        }
        send_window_mutex.unlock();
        Sleep(sleep_time);
    }

    // �ͷ�
    delete RTP_Datagram;
}

void RTP_Client::recv_thread() {
    // ���� ����ѭ�����ظ�����
    auto RTP_Datagram = new RTP_Datagram_t();

    // �����ݴ浽���ջ�����
    while (Statu != Client_Connected_Close && Statu != Client_Unknown_Error) {

        // ��ʼ��״̬�ȴ� Connect
        if (Statu == Client_init) {
            Sleep(sleep_time);
            continue;
        }

        // ��������
        memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
        int len = udp_ptr->recv((char *) RTP_Datagram, sizeof(RTP_Datagram_t), *addr_ptr);

        // ��У�� �������ֱ�Ӷ���
        if (len == -1 || !check_sum_check(RTP_Datagram)) {
            continue;
        }

        // �����ǲ�������
        if (RTP_Datagram->flag.SYN == 1) {
            // �յ� SYN ACK
            if (RTP_Datagram->flag.ACK == 1) {
                // ������������ְ���ʧ�� �ظ���ʼ��
                if (Statu == Client_Shake_Hands_1) {
                    // ѡ���С����
                    windows_size = RTP_Datagram->windows_size < windows_size ?
                                   RTP_Datagram->windows_size : windows_size;
                    // �޸�����ʱ��
                    network_delay = GetTickCount() - Shake_Hands_Send_Time + 100;
                    // ���軺������С
                    send_buf_size = windows_size * DATA_LEN * 2;
                    recv_buf_size = windows_size * DATA_LEN * 2;
                    send_buf.resize(send_buf_size);
                    recv_buf.resize(recv_buf_size);
                    // ����������
                    recv_number = RTP_Datagram->send_number;
                    // ������ջ�ַ
                    base_index = RTP_Datagram->send_number;
                    // ��ʼ������������Чֵ
                    recv_buf_effective_index.emplace_back(0, 0);
                }
                // ״̬ת��
                Statu = Client_Shake_Hands_3;
            } else {
                Statu = Client_Unknown_Error;
            }
            continue;
        }

        // �����ǲ��ǻ���
        if (RTP_Datagram->flag.FIN == 1) {
            if (RTP_Datagram->flag.ACK == 1) {
                // TODO: ����ȷ�� ɶҲ����
            } else if (RTP_Datagram->flag.ACK == 0) {
                Statu = Client_Wave_Hands_3;
            } else {
                Statu = Client_Unknown_Error;
            }
            continue;
        }

        // �����ǲ���ȷ������
        if (RTP_Datagram->flag.ACK == 1) {
            if (RTP_Datagram->recv_number <= base_index) {
                continue;
            }
            send_window_mutex.lock();
            for (auto &it: send_window) {
                if (it.send_number + it.length <= RTP_Datagram->recv_number) {
                    it.flag = true;
                } else {
                    break;
                }
            }
            send_window_mutex.unlock();
            continue;
        }

        // TODO: �������Ŀǰ��������
    }

    // �ͷ�
    delete RTP_Datagram;
}

void RTP_Client::send_thread() {
    // ���� ����ѭ�����ظ�����
    auto RTP_Datagram = new RTP_Datagram_t();

    while (Statu != Client_Connected_Close && Statu != Client_Unknown_Error) {
        // �ȿ���ǰ״̬
        switch (Statu) {
            case Client_init: {
                Sleep(sleep_time);
                continue;
            }
            case Client_Shake_Hands_1: {
                // �������� �����һ������ʧ
                if (GetTickCount() - Shake_Hands_Send_Time > network_delay) {
                    send_signals(RTP_Datagram, 0, 1, 0);
                    Shake_Hands_Send_Time = GetTickCount();
                }
                continue;
            }
            case Client_Shake_Hands_3: {
                // ��������ȷ��
                send_signals(RTP_Datagram, 1, 1, 0);
                Statu = Client_Connected;
                isConnected = true;
                thread_resend = new thread(&RTP_Client::resend_thread, this);
                thread_resend->detach();
                continue;
            }
            case Client_Wave_Hands_1: {
                // ���ͻ���
                send_signals(RTP_Datagram, 0, 0, 1);
                Sleep(network_delay);
                continue;
            }
            case Client_Wave_Hands_3: {
                // ���ͻ���ȷ��
                send_signals(RTP_Datagram, 1, 0, 1);
                Statu = Client_Wave_Hands_5;
                Sleep(network_delay * 2);
                continue;
            }
            case Client_Wave_Hands_5: {
                Statu = Client_Connected_Close;
                isConnected = false;
                continue;
            }
            default:
                break;
        }

        // �ٿ����޴���
        while (send_window.size() == windows_size) {
            Sleep(sleep_time);
        }

        send_mutex.lock();

        // ׼����������
        int effective_len = get_effective_index_len() - (get_send_number() - get_base_index());
        int send_len = (effective_len < DATA_LEN) ? effective_len : DATA_LEN;

        // ������������
        if (send_len <= 0) {
            send_mutex.unlock();
            Sleep(sleep_time);
            continue;
        }

        // ��������
        int temp_send_number = get_send_number();
        int temp_send_buf_begin = get_send_buf_effective_index_first() + (get_send_number() - get_base_index());
        if (temp_send_buf_begin >= send_buf_size) {
            temp_send_buf_begin -= send_buf_size;
        }

        send_RTP_Datagram(RTP_Datagram, temp_send_number, temp_send_buf_begin, send_len);
        // ����������
        add_send_number(send_len);

        send_mutex.unlock();

        // ��Ӵ���
        Attribute attribute{};
        attribute.time = GetTickCount();
        attribute.flag = false;
        attribute.length = send_len;
        attribute.send_number = temp_send_number;
        attribute.begin = temp_send_buf_begin;
        // ������
        send_window_mutex.lock();
        send_window.push_back(attribute);
        send_window_mutex.unlock();
    }

    // �ͷ�
    delete RTP_Datagram;
}

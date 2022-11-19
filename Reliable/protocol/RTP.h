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

#pragma pack(1)        // �����ֽڶ��뷽ʽ

struct RTP_Flag_t {
    BYTE ACK: 1;    // ȷ��
    BYTE SYN: 1;    // ��������
    BYTE FIN: 1;    // �Ͽ�����
    BYTE RES1: 5;   // ����
    BYTE RES2;      // ����
};

struct RTP_Datagram_t {
    DWORD send_number;   // �������
    DWORD recv_number;   // �������
    WORD sum_check;
    RTP_Flag_t flag;
    BYTE data[DATA_LEN];    // ����
};

#pragma pack()

// �����Э�� 3.1
class RTP {
    UDP *udp_ptr;
    // ��ʾ�����ǿͻ��˻��Ƿ�����
    bool isServer{};
    // ��ʾ�Ƿ����ֳɹ�
    bool isConnected;
    // �Է���addr
    sockaddr_in *addr_ptr;
    // ���ڴ�С
    unsigned windows_size;
    // ѭ����ʱʱ��
    unsigned sleep_time;
    // �����ӳ� ��ʱ��
    unsigned network_delay;

    // �Թ������ݼ���
    mutex send_mutex;
    mutex recv_mutex;

    // ============================= ���� ==================== //
    // �������
    DWORD send_number;
    // ���ͻ�����Ч�ֽ�
    unsigned send_buf_effective_index;
    // ���ͻ�����
    vector<char> send_buf;
    // ��ʱ���ͻ�����
    char temp_send_buf[buf_len]{};

    // �������
    DWORD recv_number;
    // ���ջ���������� recv_number ��ƫ��
    unsigned recv_buf_index_offset;
    // ���ջ�����Ч�ֽ�
    vector<pair<unsigned, unsigned>> recv_buf_effective_index;
    // ���ջ�����
    vector<char> recv_buf;
    // ============================= ���� ==================== //

    // ��ʱ������
    char temp_buf[buf_len]{};

    thread *thread_send;
    thread *thread_recv;

    // ����У����
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

    // ���У����
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

    // ����RTP_Datagram_t ����
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
        // ��������
        RTP_Datagram->sum_check = generate_sum_check(RTP_Datagram);
        RTP_Datagram_len = (int) sizeof(RTP_Datagram_t) - DATA_LEN + len;
        return RTP_Datagram;
    }

    // �ذ�
    int reply_ack(byte ack, byte syn, byte fin) {
        // �ذ�
        int len = 0;
        auto *ACK_RTP_Datagram = generate_RTP_Datagram(ack, syn, fin, temp_buf, 0, len);
        len = udp_ptr->send((char *) ACK_RTP_Datagram, len, *addr_ptr);
        delete[] ACK_RTP_Datagram;
        return len;
    }

    // �ط��߳�
    void send_again_thread(RTP_Datagram_t *RTP_Datagram, int len, bool &ack_flag) {
        DWORD start = GetTickCount();
        while (!ack_flag) {
            if (GetTickCount() - start >= network_delay * 5) {
                udp_ptr->send((char *) RTP_Datagram, len, *addr_ptr);
                start = GetTickCount();
                cerr << "���� " << RTP_Datagram->send_number << " �ش�... " << endl;
            }
        }
    }

    void recv_thread() {
        int len;
        auto *RTP_Datagram = new RTP_Datagram_t();

        // �����ݴ浽���ջ�����
        while (isConnected) {
            memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
            len = udp_ptr->recv((char *) RTP_Datagram, sizeof(RTP_Datagram_t), *addr_ptr);
            if (len != -1 && check_sum_check(RTP_Datagram)) {
                // �����ǲ����ش�
                if (RTP_Datagram->send_number < recv_number) {
                    // �ذ�
                    reply_ack(1, 0, 0);
                    continue;
                }

                cout << "���� " << RTP_Datagram->send_number << " �ɹ�" << endl;

                /* ������ ���ڽ��ջ������� ���ǻ������������ */

                // ������ĳ���
                unsigned data_len = len - (sizeof(RTP_Datagram_t) - DATA_LEN);

                // ��û���������ʼλ��
                unsigned start, end;
                while (true) {
                    // ������������ǰ����
                    recv_mutex.lock();

                    start = RTP_Datagram->send_number - recv_buf_index_offset;
                    end = start + data_len;

                    // ���������Է��¾�����
                    if (end <= recv_buf.size()) {
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
                recv_number = recv_buf_index_offset + recv_buf_effective_index[0].second;

                // �����������ݺ����
                recv_mutex.unlock();

                // �ذ�
                reply_ack(1, 0, 0);
            }
        }

        delete RTP_Datagram;
    }

    void send_thread() {
        // �����ͻ����������ݷ��ͳ�ȥ
        while (isConnected) {
            int len = 0;
            int send_len = (send_buf_effective_index < DATA_LEN) ? (int) send_buf_effective_index : DATA_LEN;

            if (send_len == 0) {
                Sleep(sleep_time);
                continue;
            }

            // ������������ǰ����
            send_mutex.lock();

            // �������ݲ�����
            copy(send_buf.begin(), send_buf.begin() + send_len, temp_send_buf);
            auto *RTP_Datagram_send = generate_RTP_Datagram(0, 0, 0, temp_send_buf, send_len, len);
            udp_ptr->send((char *) RTP_Datagram_send, len, *addr_ptr);

            // ���� ��ʱ��
            bool ack_flag = false;
            thread timer(&RTP::send_again_thread, this, RTP_Datagram_send, len, ref(ack_flag));
            timer.detach();

            auto *RTP_Datagram = new RTP_Datagram_t();
            memset(RTP_Datagram, 0, sizeof(RTP_Datagram_t));
            len = udp_ptr->recv((char *) RTP_Datagram, sizeof(RTP_Datagram_t), *addr_ptr);

            // �յ�ACK��Ӧ�� ��� RTP_Datagram_send
            ack_flag = true;
            delete RTP_Datagram_send;

            if (len == -1) {
                delete RTP_Datagram;
                // �����������ݺ����
                send_mutex.unlock();
                continue;
            } else {
                if (!check_sum_check(RTP_Datagram) || RTP_Datagram->flag.ACK != 1 ||
                    RTP_Datagram->recv_number != send_number + send_len) {
                    delete RTP_Datagram;
                    // �����������ݺ����
                    send_mutex.unlock();
                    continue;
                } else {
                    cout << "����" << RTP_Datagram->recv_number << " �ɹ�" << endl;
                    // �����շ��� �Լ� ������
                    send_number += send_len;
                    copy(send_buf.begin() + send_len, send_buf.end(), send_buf.begin());
                    send_buf_effective_index -= send_len;

                    delete RTP_Datagram;
                    // �����������ݺ����
                    send_mutex.unlock();
                }
            }
        }
    }

public:
    RTP() {
        random_device rd;
        // ========== ����һ�������
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

        // =============== ��ʼ����
        int len;
        auto *RTP_Datagram = new RTP_Datagram_t();

        // ���ֵ�һ��
        DWORD start = GetTickCount();
        if (reply_ack(0, 1, 0) == -1) {
            return -1;
        }
        cout << "client ��һ������ �ɹ� " << endl;

        // ���ֵڶ���
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
        cout << "client �ڶ������� �ɹ� " << endl;

        // ���ֵ�����
        if (reply_ack(1, 1, 0) == -1) {
            return -1;
        }
        cout << "client ���������� �ɹ� " << endl;

        // ���ֳɹ���, ���������߳����ڷ�������
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

        // =============== ��ʼ����
        int len;
        auto *RTP_Datagram = new RTP_Datagram_t();

        // ���ֵ�һ��
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
        cout << "server ��һ������ �ɹ� " << endl;

        // ���ֵڶ���
        DWORD start = GetTickCount();
        if (reply_ack(1, 1, 0) == -1) {
            return -1;
        }
        cout << "server �ڶ������� �ɹ� " << endl;

        // ���ֵ�����
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
        cout << "server �ڶ������� �ɹ� " << endl;

        // ���ֳɹ���, ���������߳����ڽ�������
        isConnected = true;
        thread_recv = new thread(&RTP::recv_thread, this);
        thread_recv->detach();
        return 0;
    }

    int recv(char *buf, int len, int timeout = INT32_MAX) {
        DWORD start = GetTickCount();
        while (true) {
            // ����ʱ
            if (GetTickCount() - start >= timeout) {
                return -1;
            }

            if (recv_buf_effective_index[0].second == 0) {
                Sleep(sleep_time);
                continue;
            }

            // ������������ǰ����
            recv_mutex.lock();

            int data_len = (recv_buf_effective_index[0].second < len) ? (int) recv_buf_effective_index[0].second : len;
            // ����Ԫ��
            copy(recv_buf.cbegin(), recv_buf.cbegin() + data_len, buf);
            // ��������
            copy(recv_buf.begin() + data_len, recv_buf.end(), recv_buf.begin());
            // ������������
            recv_buf_index_offset += data_len;
            for (auto &pair: recv_buf_effective_index) {
                pair.first -= data_len;
                pair.second -= data_len;
            }
            recv_buf_effective_index[0].first = 0;

            // �����������ݺ����
            recv_mutex.unlock();
            return data_len;
        }
    }

    int send(char *buf, int len, int timeout = INT32_MAX) {
        DWORD start = GetTickCount();
        while (true) {
            // д��ʱ
            if (GetTickCount() - start >= timeout) {
                return -1;
            }
            // TODO: ��д ���⻺����̫С�����޷���������
            if ((send_buf.size() - send_buf_effective_index) < len) {
                Sleep(sleep_time);
                continue;
            }
            // ������������ǰ����
            send_mutex.lock();
            // ����Ԫ��
            copy(buf, buf + len, send_buf.begin() + send_buf_effective_index);
            // ������������
            send_buf_effective_index += len;
            // �����������ݺ����
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

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

#pragma pack(1)        // �����ֽڶ��뷽ʽ

// 1 Bytes
struct Flag {
    BYTE ACK: 1;    // ȷ��
    BYTE SYN: 1;    // ��������
    BYTE FIN: 1;    // �Ͽ�����
    BYTE RES1: 5;   // ����
};

// 1452 Bytes
struct RTP_Datagram_t {
    DWORD send_number;   // �������
    DWORD recv_number;   // �������
    WORD sum_check;      // У���
    Flag flag;           // ��־λ
    BYTE windows_size;   // ���ڴ�С
    BYTE data[DATA_LEN]; // ������
};

// ������״̬ö��
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

// �ͻ���״̬ö��
enum ClientStatus {
    Client_init = 0,

    Client_Shake_Hands_1,
    Client_Shake_Hands_3,

    Client_Connected,

    Client_Wave_Hands_1,
    Client_Wave_Hands_3,
    Client_Wave_Hands_5, // �ȴ�һ��������ʱ, ��״̬Ϊ�ı���ɹ�����

    Client_Connected_Close,

    Client_Unknown_Error,
};

// ��������
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
    // ============================= ��ʼ���� ==================== //
    // UDP����
    UDP *udp_ptr = new UDP();
    // �Է���addr connect����acceptʱ����
    sockaddr_in *addr_ptr = nullptr;
    // ѭ����ʱʱ��
    DWORD sleep_time = 0;
    // �������
//    DWORD send_number = rand() % 100;
    DWORD send_number = 0;
    // ���ͻ�������Ч�ֽ�
    pair<int, int> send_buf_effective_index{0, 0};
    int send_buf_effective_num = 0;
    // ============================= ���ָ������� ==================== //
    // �Ƿ����� �ṩ��send �� recv ʹ��
    bool isConnected = false;
    // �ϴ����ַ��͵�ʱ��
    DWORD Shake_Hands_Send_Time = -1;
    // ���ڴ�С
    DWORD windows_size = 4;
    // �����ӳ� ms
    DWORD network_delay = 1000;
    // �������
    DWORD recv_number = 0;
    // ��ȷ�����ݻ�ַ
    DWORD base_index = 0;
    // ���ͻ����� ����˫����С��������
    vector<char> send_buf;
    int send_buf_size = 0;
    // ���ջ����� ����˫����С��������
    vector<char> recv_buf;
    int recv_buf_size = 0;
    // ���ջ�����Ч�ֽ�
    vector<pair<int, int>> recv_buf_effective_index;
    // ============================= �޳�ʼֵ ==================== //
    // ��ʱ������
    char temp_buf[DATA_LEN]{};
    // �շ��߳�
    thread *thread_send = nullptr;
    thread *thread_recv = nullptr;
    thread *thread_resend = nullptr;

    // �Թ������ݼ���
    mutex send_mutex;
    mutex recv_mutex;
    mutex send_window_mutex;
    mutex share_data_mutex;

    // ======== �ṩһЩget set ���� �������Ĺ��� TODO:��������̫�� ======== //
    DWORD get_send_number() {
        // ����ʱ���� ����ʱ����
        unique_lock<mutex> lock(share_data_mutex);
        return send_number;
    }

    void add_send_number(DWORD number) {
        // ����ʱ���� ����ʱ����
        unique_lock<mutex> lock(share_data_mutex);
        send_number += number;
    }

    int get_effective_index_len() {
        // ����ʱ���� ����ʱ����
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
    // ����У����
    static WORD generate_sum_check(RTP_Datagram_t *RTP_Datagram);

    // ���У����
    static bool check_sum_check(RTP_Datagram_t *RTP_Datagram);

    // ����RTP_Datagram_t ����
    int generate_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, char *buf, int len,
                              BYTE ACK, BYTE SYN, BYTE FIN, DWORD number) const;

    // �����ź�
    int send_signals(RTP_Datagram_t *RTP_Datagram, byte ack, byte syn, byte fin);

    // ����һ�� buf ��Ч����, ��������䵽temp_buf ���� ������
    int send_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, DWORD number, int begin, int length);

    // ============================= ������д ==================== //
    virtual void recv_thread() = 0;

    virtual void send_thread() = 0;

public:

    int recv(char *buf, int len, int timeout = INT32_MAX) {
        DWORD start = GetTickCount();
        while (true) {
            // ����ʱ
            if (!isConnected || GetTickCount() - start >= timeout) {
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
            base_index += data_len;
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
            if (!isConnected || GetTickCount() - start >= timeout) {
                return -1;
            }

            send_mutex.lock();
            // ��������������
            if ((send_buf_size - get_effective_index_len()) < len) {
                send_mutex.unlock();
                Sleep(sleep_time);
                continue;
            }

            // ȷ���˿�send_buf_effective_index.second��ֵ��ȷ
            int temp_send_buf_effective_index_second = get_send_buf_effective_index_second();
            // ����Ԫ��
            int buf_end_2_effective_index_end = (int) send_buf_size - temp_send_buf_effective_index_second;
            if (buf_end_2_effective_index_end > len) {
                copy(buf, buf + len, send_buf.begin() + temp_send_buf_effective_index_second);
            } else {
                copy(buf, buf + buf_end_2_effective_index_end, send_buf.begin() + temp_send_buf_effective_index_second);
                copy(buf + buf_end_2_effective_index_end, buf + len, send_buf.begin());
            }
            // ������������
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
    // �����״̬
    ServerStatus Statu;
    // �źŶ��� ���ڽ��շ����̼߳�ͨ��
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
        // �������պͷ����߳�
        thread_recv = new thread(&RTP_Server::recv_thread, this);
        thread_send = new thread(&RTP_Server::send_thread, this);
        thread_recv->detach();
        thread_send->detach();
        // ��ʼ�� �Է���ַ
        addr_ptr = new sockaddr_in();
        // ��������ʼ����
        Statu = Server_listen;
        return 0;
    }

    int accept() {
        // ����״̬�ж� �Ƿ����ӳɹ�
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
    // �ͻ���״̬
    ClientStatus Statu;
    // ���ʹ�������
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
        // �������պͷ����߳�
        thread_recv = new thread(&RTP_Client::recv_thread, this);
        thread_send = new thread(&RTP_Client::send_thread, this);
        thread_recv->detach();
        thread_send->detach();
        // ��ʼ��
        addr_ptr = UDP::get_SockAddr_In(host, port);

        // ��ʼ��ʱ ��ʱ����ʧ��
        DWORD start = GetTickCount();
        // ����״̬�ж� �Ƿ����ӳɹ�
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

        // �ͻ���׼������
        Statu = Client_Wave_Hands_1;
        DWORD start = GetTickCount();
        // ����״̬�ж� �Ƿ����ӳɹ�
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
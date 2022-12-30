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
#define DATA_LEN 10000

#pragma pack(1)        // �����ֽڶ��뷽ʽ

// 1 Byte
struct RTP_Flag_t {
    BYTE ACK: 1;    // ȷ��
    BYTE SYN: 1;    // ��������
    BYTE FIN: 1;    // �Ͽ�����
    BYTE RES1: 5;   // ����
};

// 12 Bytes
struct RTP_Head_t {
    DWORD send_num;      // �������
    DWORD recv_num;      // �������
    WORD check_sum;      // У���
    RTP_Flag_t flag;     // ��־λ
    BYTE windows_num;    // ���ڴ�С
};

// DATA_LEN Bytes
struct RTP_Data_t {
    BYTE data[DATA_LEN]; // ������
};

// 12 + DATA_LEN Bytes
struct RTP_Datagram_t {
    RTP_Head_t head;
    RTP_Data_t data;
};

// ��������
struct Attribute_t {
    DWORD time;
    int index;
    int send_number;
    bool flag;
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

#pragma pack()

// RTP �ɿ��������
class RTP {
protected:
    // UDP ����
    UDP *udp_ptr = new UDP();
    // connect/accept ����
    sockaddr_in *addr_ptr = nullptr;
    // �Ƿ����� �ṩ��send �� recv ʹ��
    bool isConnected = false;
    // ѭ����ʱʱ��
    DWORD sleep_time = 0;
    // ���ڴ�С ����ӵ������
    DWORD windows_number = 5;
    // ��󴰿ڴ�С ����ӵ������
    DWORD max_windows_number = 5;
    // ���ش�
    DWORD quick_resend_number = 0;
    // ��������С
    DWORD buffers_number = 0;
    // �����ӳ� ��ʼֵ1s
    DWORD network_delay = 1000;
    // �ϴ����ַ��͵�ʱ��
    DWORD Shake_Hands_Send_Time = -1;
    // ����/���ͻ�ַ
    DWORD recv_base = 0;
    DWORD send_base = rand() % 100;
    // ����/�������
    DWORD recv_number = 0;
    DWORD send_number = send_base;
    // ���ͻ�����
    vector<pair<int, RTP_Data_t>> send_buffers;
    int free_buffer_head = 0;
    int free_buffer_number = 0;
    int valid_buffer_head = 0;
    // ���ջ�����
    vector<char> recv_buffers;
    list<pair<int, int>> valid_buffer_index{{0, 0}};
    // �շ��߳�
    thread *thread_send = nullptr;
    thread *thread_recv = nullptr;
    thread *thread_resend = nullptr;
    // �Թ������ݼ���
    mutex send_mutex;
    mutex recv_mutex;

    // RTP �ṩ������Ľӿ�
    int recv_RTP_Datagram(RTP_Datagram_t *RTP_Datagram);

    int send_RTP_Signals(RTP_Datagram_t *RTP_Datagram, RTP_Head_t head);

    int send_RTP_Datagram(RTP_Datagram_t *RTP_Datagram, RTP_Head_t head, int index);

public:
    // RTP �ṩ���ϲ�Ľӿ�
    int recv(char *buf, int len, int timeout = INT32_MAX);

    int send(char *buf, int len, int timeout = INT32_MAX);

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
    queue<RTP_Flag_t> signals_queue;

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
    list<Attribute_t> send_windows_list;

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
        while (free_buffer_number != buffers_number) {
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
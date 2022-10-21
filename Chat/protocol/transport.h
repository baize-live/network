#ifndef CHAT_TRANSPORT_H
#define CHAT_TRANSPORT_H

#include <string>
#include "winsock2.h"
#include <vector>

using namespace std;

const int buf_len = 2048;

class tcp {
public:
    static void send(SOCKET client, const vector<char> &out_buf) {
        char buf[buf_len];
        int sum_len = (int) out_buf.size();
        for (int i = 0, pos_num = 0; i < sum_len; i += buf_len, ++pos_num) {
            memcpy(buf, &sum_len, sizeof(sum_len));
            memcpy(buf + sizeof(sum_len), &pos_num, sizeof(pos_num));
            int temp = sizeof(sum_len) + sizeof(pos_num);
            int size;
            int vec_buf = (int) (buf_len - temp);
            if (sum_len < vec_buf * (pos_num + 1)) {
                size = sum_len - vec_buf * pos_num;
            } else {
                size = vec_buf;
            }
            auto begin = out_buf.cbegin() + vec_buf * pos_num;
            copy(begin, begin + size, buf + temp);
            ::send(client, buf, size + temp, 0);
        }
    }

    static void recv(SOCKET client, vector<char> &in_buf) {
        char buf[buf_len];
        in_buf.resize(0);
        while (true) {
            int len = ::recv(client, buf, buf_len, 0);
            int sum_len = 0;
            int pos_num = 0;
            memcpy(&sum_len, buf, sizeof(sum_len));
            memcpy(&pos_num, buf + sizeof(sum_len), sizeof(pos_num));
            int temp = sizeof(sum_len) + sizeof(pos_num);

            int size = len - temp;

            for (int i = sizeof(sum_len) + sizeof(pos_num); i < len; i++) {
                in_buf.push_back(buf[i]);
            }

            if (len < buf_len) {
                break;
            }
        }
    }
};

class tcp_server {
private:
    SOCKET server;

public:
    explicit tcp_server(short port) {
        WSADATA wsd{};
        // ��ʼ�� socket���� //��һ��������dll�汾�ţ��ڶ��������Ǵ�����socket���� ��Ҫȷ���Ƿ��ʼ���ɹ�
        if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
//        WSAStartup���������
//        WSASYSNOTREADY  ����ͨ�����²��������ϵͳû׼����
//        WSAVERNOTSUPPORTED  Socketʵ���ṩ�汾��socket��Ҫ�İ汾����
//        WSAEINPROGRESS  һ��������Socket�������ڽ���
//        WSAEPROCLIM  Socket��ʵ�ֳ���Socket֧�ֵ�����������
//        WSAEFAULT   lpWSAData��������һ���Ϸ���ָ��
            return;
        }
        cout << "WSAStartup  success.." << endl;
        // ����socket
        server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server == INVALID_SOCKET) {
            return;
        }
        cout << "create socket.." << endl;

        // �����׽��ֵ�ַ
        sockaddr_in server_addr{};
        server_addr.sin_family = PF_INET;
        server_addr.sin_port = htons(port);//�˿�
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");//����ĵ�ַ
        if (bind(server, (sockaddr *) &server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            return;
        }
        cout << "bind success.." << endl;

        // ��ʼ����
        if (listen(server, 10) == SOCKET_ERROR) {
            return;
        }
        cout << "listen success.." << endl;

        cout << "server host: 127.0.0.1" << endl;
        cout << "server port: " << port << endl;
    }

    // ����һ���ͻ�������
    SOCKET tcp_accept() const {
        sockaddr_in client_sin{};
        int len = sizeof(client_sin);
        SOCKET client = accept(server, (sockaddr *) &client_sin, &len);
        return client;
    }

    void tcp_close() const {
        closesocket(server);
        WSACleanup();
    }
};

class tcp_client {
private:
    SOCKET client;

public:
    tcp_client() {
        WSADATA wsd{};
        // ��ʼ�� socket���� //��һ��������dll�汾�ţ��ڶ��������Ǵ�����socket���� ��Ҫȷ���Ƿ��ʼ���ɹ�
        if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
            return;
        }
        // ����socket����
        client = socket(AF_INET, SOCK_STREAM, 0);
    }

    SOCKET tcp_connect(const string &host, const short port, bool &flag) const {
        // ���÷�������Ϣ
        sockaddr_in client_addr{};
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(port);
        char *host_temp = new char[host.size() + 1];
        copy(host.cbegin(), host.cend(), host_temp);
        host_temp[host.size()] = '\0';

        client_addr.sin_addr.S_un.S_addr = inet_addr(host_temp);
        if (connect(client, (sockaddr *) &client_addr, sizeof(client_addr)) == 0) {
            flag = true;
        }
        delete[] host_temp;
        return client;
    }

    void tcp_close() const {
        closesocket(client);
        WSACleanup();
    }
};

#endif //CHAT_TRANSPORT_H

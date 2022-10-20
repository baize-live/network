#ifndef CHAT_PROTOCOL_H
#define CHAT_PROTOCOL_H

#include <string>
#include <vector>
#include <map>
#include "transport.h"
#include "config.h"
#include <iostream>

using namespace std;

class protocol {
protected:
    map<string, string> headers;
    vector<char> data;

public:
    protocol() = default;

    void set_header(const string &key, const string &value) {
        headers[key] = value;
    }

    void set_text(const string &text) {
        data.resize(0);
        for (const auto &ch: text) {
            data.push_back(ch);
        }
    }

    void set_file(FILE *file) {

    }

    void set_data(const vector<char> &buf) {
        data.resize(0);
        for (const auto &ch: buf) {
            data.push_back(ch);
        }
    }

    void set_bytes(const vector<char> &buf) {
        if (buf.empty()) {
            return;
        }
        string key;
        string value;
        bool flag = true;
        auto it = buf.cbegin();
        // ����headers
        headers.clear();
        for (; it != buf.cend(); ++it) {
            if (*it == '\r') {
                if (*(++it) == '\n') {
                    ++it;
                    break;
                } else {
                    return;
                }
            }
            if (*it == '\n') {
                headers[key] = value;
                key = "";
                value = "";
                flag = true;
                continue;
            }
            if (*it == 0x00) {
                flag = false;
                continue;
            }
            if (flag) {
                key += *it;
            } else {
                value += *it;
            }
        }
        // ����data
        data.resize(0);
        for (; it != buf.cend(); ++it) {
            data.push_back(*it);
        }
    }

    string get_header(const string &key) {
        if (headers.count(key)) {
            return headers[key];
        } else {
            return "";
        }
    }

    string get_text() {
        string text;
        for (char i: data) {
            text += i;
        }
        return text;
    }

//    FILE *get_file() {
//
//    }

    vector<char> get_data() {
        return data;
    }

    vector<char> get_bytes() {
        vector<char> buf;
        // ���� headers
        for (const auto &header: headers) {
            string key = header.first;
            string value = header.second;
            for (const auto &ch: key) {
                buf.push_back(ch);
            }
            buf.push_back(0x00);
            for (const auto &ch: value) {
                buf.push_back(ch);
            }
            buf.push_back('\n');
        }
        buf.push_back('\r');
        buf.push_back('\n');

        // ���� data
        for (const auto &ch: data) {
            buf.push_back(ch);
        }
        return buf;
    }

    ~protocol() = default;
};

class Response : public protocol {
public:
    Response() {
        // ���Э������
        headers["author"] = "BZ";
    };

    string get_res() {
        return headers["res"];
    }

    void set_res(string res) {
        headers["res"] = res;
    }

    void response(SOCKET client) {
        // ����
        vector<char> out_buf = get_bytes();
        tcp::send(client, out_buf);
        closesocket(client);
    }
};

class Request : public protocol {
public:
    Request() {
        // ���Э������
        headers["author"] = "BZ";
    }

    explicit Request(map<string, string> &headers) {
        // ���Э������
        this->headers["author"] = "BZ";
        for (const auto &header: headers) {
            this->headers[header.first] = header.second;
        }
    }

    Response request(const string &host, const short &port) {
        tcp_client tcp_client;
        bool is_connected = false;
        SOCKET client = tcp_client.tcp_connect(host, port, is_connected);
        Response response;
        if (is_connected) {
            // ����
            vector<char> out_buf = get_bytes();
            tcp::send(client, out_buf);
            // ����
            vector<char> in_buf;
            tcp::recv(client, in_buf);
            response.set_bytes(in_buf);
        } else {
            // �����쳣
            response.set_header("res", Res::Is_not_connected);
        }
        // �ر�����
        tcp_client.tcp_close();
        return response;
    }
};

#endif //CHAT_PROTOCOL_H

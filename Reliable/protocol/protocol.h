#ifndef CHAT_PROTOCOL_H
#define CHAT_PROTOCOL_H

#include <map>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include "RTP.h"

using namespace std;

// 应用层协议
class protocol {
protected:
    map<string, string> headers;
    vector<char> body;
    char temp_buf[DATA_LEN]{};

public:
    protocol() = default;

    void set_header(const string &key, const string &value) {
        headers[key] = value;
    }

    void set_text(const string &text) {
        body.resize(0);
        for (const auto &ch: text) {
            body.push_back(ch);
        }
    }

    void set_file(FILE *file) {

    }

    void set_body(const vector<char> &buf) {
        body.resize(buf.size());
        copy(buf.begin(), buf.end(), body.begin());
    }

    void set_bytes(const vector<char> &buf) {
        if (buf.empty()) {
            return;
        }
        string key;
        string value;
        bool flag = true;
        auto it = buf.cbegin();
        // 解析headers
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
        // 解析data
        body.resize(0);
        for (; it != buf.cend(); ++it) {
            body.push_back(*it);
        }
    }

    void set_bytes(char *buf, int len) {
        if (len < 1) {
            return;
        }
        vector<char> vec(len);
        copy(buf, buf + len, vec.begin());
        set_bytes(vec);
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
        for (char i: body) {
            text += i;
        }
        return text;
    }

    string get_author() {
        return headers["author"];
    }

    vector<char> get_body() {
        return body;
    }

    vector<char> get_bytes() {
        vector<char> buf;
        // 遍历 headers
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

        // 遍历 body
        for (const auto &ch: body) {
            buf.push_back(ch);
        }
        return buf;
    }

    ~protocol() = default;
};

class Response : public protocol {
public:
    Response() {
        // 标记协议作者
        headers["author"] = "BZ";
    };

    string get_res() {
        return headers["res"];
    }

    void set_res(string res) {
        headers["res"] = std::move(res);
    }

    void response(RTP_Server &server) {
        // 发送
        vector<char> out_buf = get_bytes();
        copy(out_buf.begin(), out_buf.end(), temp_buf);
        server.send(temp_buf, (int) out_buf.size());
    }
};

class Request : public protocol {
public:
    Request() {
        // 标记协议作者
        headers["author"] = "BZ";
    }

    explicit Request(map<string, string> &headers) {
        // 标记协议作者
        this->headers["author"] = "BZ";
        for (const auto &header: headers) {
            this->headers[header.first] = header.second;
        }
    }

    string get_method() {
        return headers["method"];
    }

    void set_method(string method) {
        headers["method"] = std::move(method);
    }

    // TODO: 现在服务端无响应
    Response request(RTP_Client &client) {
        Response response;
        // 发送
        vector<char> out_buf = get_bytes();
        copy(out_buf.begin(), out_buf.end(), temp_buf);
        client.send(temp_buf, (int) out_buf.size());
        return response;
    }
};

#endif //CHAT_PROTOCOL_H

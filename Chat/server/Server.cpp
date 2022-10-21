#include <thread>
#include <iostream>
#include "../protocol/transport.h"
#include "../protocol/protocol.h"
#include <vector>
#include <set>
#include <list>

using namespace std;


class Server {
    class message {
    public:
        string username_src;
        string username_des;
        string info;
        string time;

        string tostring_user_info() const {
            return info + "\n" + time + "\n\r";
        }

        string tostring_all_user_info() const {
            return username_src + "\n" + info + "\n" + time + "\n\r";
        }
    };

    // =========================== //
    // 保存所有注册过的用户
    set<string> usernames;
    // 保存每个用户收到的消息
    map<string, list<message>> user_message_list;

    // ================= Service
    void handle_register(Request &req, Response &res) {
        string username = req.get_header("username");
        // 初始化用户信息
        usernames.insert(username);
        // 初始化消息队列
        list<message> message_list;
        message_list = user_message_list[username];
        user_message_list[username] = message_list;
        cout << "user " << username << " register success..." << endl;
    }

    void handle_lookup_all_users(Request &req, Response &res) {
        string self = req.get_header("username");
        string username_str;
        for (auto &username: usernames) {
            if (username != self) {
                username_str += username;
                username_str += "\n";
            }
        }
        username_str += "\r";
        res.set_text(username_str);
        cout << "user " << self << " lookup_all_users success..." << endl;
    }

    void handle_send_info_to_user(Request &req, Response &res) {
        message msg;
        msg.username_src = req.get_header("username_src");
        msg.username_des = req.get_header("username_des");
        msg.info = req.get_header("info");
        msg.time = req.get_header("time");
        user_message_list[msg.username_des].push_back(msg);
        cout << msg.username_src << " send_info to " << msg.username_des << " success..." << endl;
    }

    void handle_recv_info_from_user(Request &req, Response &res) {
        message msg;
        msg.username_src = req.get_header("username_src");
        msg.username_des = req.get_header("username_des");
        msg.time = req.get_header("time");

        list<message> *message_list = &user_message_list[msg.username_src];
        string text;
        auto it = message_list->begin();
        while (it != message_list->end()) {
            if (it->username_src == msg.username_des) {
                text += it->tostring_user_info();
                it = message_list->erase(it);
                continue;
            }
            ++it;
        }

        if (!text.empty()) {
            res.set_text(text);
            cout << msg.username_src << " recv_info from " << msg.username_des << " success..." << endl;
        }
    }

    void handle_recv_info_from_all_user(Request &req, Response &res) {
        string username = req.get_header("username");
        list<message> *message_list = &user_message_list[username];
        string text;
        auto it = message_list->begin();
        while (it != message_list->end()) {
            text += it->tostring_all_user_info();
            ++it;
        }
        res.set_text(text);
        cout << username << " recv_info_from_all_user success..." << endl;
    }

    // ================= Controller
    void handle_request(SOCKET client) {
        vector<char> in_buf;
        tcp::recv(client, in_buf);
        Request request;
        request.set_bytes(in_buf);
        Response response;
        if (request.get_author() != "BZ") {
            response.set_res(Res::Invalid);
        } else {
            string method = request.get_method();
            // 默认成功
            response.set_res(Res::Success);

            if (method == Method::method_register) {
                handle_register(request, response);
            } else if (method == Method::method_lookup_all_users) {
                handle_lookup_all_users(request, response);
            } else if (method == Method::method_send_info_to_user) {
                handle_send_info_to_user(request, response);
            } else if (method == Method::method_recv_info_from_user) {
                handle_recv_info_from_user(request, response);
            } else if (method == Method::method_recv_info_from_all_user) {
                handle_recv_info_from_all_user(request, response);
            } else {
                response.set_res(Res::Invalid);
            }
        }
        response.response(client);
    }

public :
    Server() {
        tcp_server tcp_server(Config::port);
        cout << "服务器启动成功 " << endl;

        while (true) {
            SOCKET client = tcp_server.tcp_accept();
            thread handle_thread(&Server::handle_request, this, client);
            handle_thread.join();
        }
    }
};


int main() {
    Server();
}

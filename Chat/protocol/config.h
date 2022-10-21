#ifndef CHAT_CONFIG_H
#define CHAT_CONFIG_H

#include <string>

using namespace std;

class Res {
public:
    static string Is_not_connected;
    static string Success;
    static string Invalid;
};

class Method {
public:
    static string unknown;
    static string method_register;
    static string method_lookup_all_users;
    static string method_send_info_to_user;
    static string method_recv_info_from_user;
    static string method_recv_info_from_all_user;
};

class Config {
public:
    static const char *host;
    static const short port;
};


#endif //CHAT_CONFIG_H

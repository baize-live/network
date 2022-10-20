#ifndef CHAT_CONTROL_H
#define CHAT_CONTROL_H

#include <string>

class Control {
public:
    static void handle_welcome(const string &num);

    static void handle_register(const string &username);

    static void handle_main(const string &num);

    static void handle_person_info(int num, const vector<string> &usernames);

    static void recv_user_info_thread();

    static void send_user_info_thread(const string& info);

    static void stop_loop_recv_info();

};

#endif //CHAT_CONTROL_H

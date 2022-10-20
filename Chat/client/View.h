#ifndef CHAT_VIEW_H
#define CHAT_VIEW_H

#include <string>
#include <vector>

using namespace std;

class View {
public:
    // 显示注册界面
    static void show_register_window();

    // 显示主窗口
    static void show_main_window();

    // 显示欢迎界面
    static void show_welcome_window();

    // 显示人员界面
    static void show_person_info(const vector<string> &usernames);

    // 显示聊天界面
    static void show_chat_window(const string &username);

};

#endif //CHAT_VIEW_H

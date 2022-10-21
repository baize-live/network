#include <thread>
#include "View.h"
#include "Control.h"
#include "iostream"
#include <string>

using namespace std;

void View::show_main_window() {
    while (true) {
        system("cls");
        cout << "******************************" << endl;
        cout << "欢迎使用 BaiZeChat" << endl;
        cout << "1. 查看当前人员信息" << endl;
        cout << "2. 查看当前群聊信息" << endl;
        cout << "3. 查看是否收到信息" << endl;
        cout << "4. 退出" << endl;
        cout << "******************************" << endl;
        cout << "请输入您的选择: ";

        string num;
        cin >> num;

        Control::handle_main(num);
    }
}

void View::show_register_window() {
    while (true) {
        system("cls");
        cout << "******************************" << endl;
        cout << "欢迎注册 BaiZeChat" << endl;
        cout << "******************************" << endl;
        cout << "输入您的用户名(不超过10个字符): ";
        string username;
        cin >> username;
        Control::handle_register(username);
        system("pause");
        break;
    }
}

void View::show_welcome_window() {
    while (true) {
        system("cls");
        cout << "**********************" << endl;
        cout << "欢迎使用 BaiZeChat" << endl;
        cout << "请您先注册 " << endl;
        cout << "1. 注册" << endl;
        cout << "2. 退出" << endl;
        cout << "3. 查看开发者信息" << endl;
        cout << "**********************" << endl;
        cout << "输入您的选择: ";
        string num;
        cin >> num;
        Control::handle_welcome(num);
    }
}

void View::show_person_info(const vector<string> &usernames) {
    while (true) {
        system("cls");
        cout << "**********************" << endl;
        cout << "欢迎使用 BaiZeChat" << endl;
        cout << "用户信息如下: " << endl;
        int i = 0;
        for (const auto &username: usernames) {
            cout << ++i << ". " << username << endl;
        }
        cout << ++i << ". 回到主界面" << endl;
        cout << "**********************" << endl;
        cout << "输入您的选择: (选择用户发起聊天)";

        string num;
        cin >> num;
        char i_str[10];
        sprintf(i_str, "%d", i);
        if (num == i_str) {
            break;
        }
        try {
            int num_int = stoi(num);
            Control::handle_person_info(num_int, usernames);
        } catch (exception &e) {
            cout << "请输入正确的数字..." << endl;
            system("pause");
        }
    }
}

void View::show_chat_window(const string &username) {
    system("cls");
    cout << "**********************" << endl;
    cout << "欢迎使用 BaiZeChat" << endl;
    cout << "您可以通过输入 exit_BaiZeChat 退出该聊天室" << endl;
    cout << username << endl << endl;

    // 接收信息线程
    auto recv_func = []() {
        Control::recv_user_info_thread();
    };
    thread recv_thread(recv_func);

    // 发送信息
    while (true) {
        cout << ">> ";
        string info;
        cin >> info;
        if (info == "exit_BaiZeChat") {
            Control::stop_loop_recv_info();
            recv_thread.join();
            break;
        }
        Control::send_user_info_thread(info);
    }
}

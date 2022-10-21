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
        cout << "��ӭʹ�� BaiZeChat" << endl;
        cout << "1. �鿴��ǰ��Ա��Ϣ" << endl;
        cout << "2. �鿴��ǰȺ����Ϣ" << endl;
        cout << "3. �鿴�Ƿ��յ���Ϣ" << endl;
        cout << "4. �˳�" << endl;
        cout << "******************************" << endl;
        cout << "����������ѡ��: ";

        string num;
        cin >> num;

        Control::handle_main(num);
    }
}

void View::show_register_window() {
    while (true) {
        system("cls");
        cout << "******************************" << endl;
        cout << "��ӭע�� BaiZeChat" << endl;
        cout << "******************************" << endl;
        cout << "���������û���(������10���ַ�): ";
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
        cout << "��ӭʹ�� BaiZeChat" << endl;
        cout << "������ע�� " << endl;
        cout << "1. ע��" << endl;
        cout << "2. �˳�" << endl;
        cout << "3. �鿴��������Ϣ" << endl;
        cout << "**********************" << endl;
        cout << "��������ѡ��: ";
        string num;
        cin >> num;
        Control::handle_welcome(num);
    }
}

void View::show_person_info(const vector<string> &usernames) {
    while (true) {
        system("cls");
        cout << "**********************" << endl;
        cout << "��ӭʹ�� BaiZeChat" << endl;
        cout << "�û���Ϣ����: " << endl;
        int i = 0;
        for (const auto &username: usernames) {
            cout << ++i << ". " << username << endl;
        }
        cout << ++i << ". �ص�������" << endl;
        cout << "**********************" << endl;
        cout << "��������ѡ��: (ѡ���û���������)";

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
            cout << "��������ȷ������..." << endl;
            system("pause");
        }
    }
}

void View::show_chat_window(const string &username) {
    system("cls");
    cout << "**********************" << endl;
    cout << "��ӭʹ�� BaiZeChat" << endl;
    cout << "������ͨ������ exit_BaiZeChat �˳���������" << endl;
    cout << username << endl << endl;

    // ������Ϣ�߳�
    auto recv_func = []() {
        Control::recv_user_info_thread();
    };
    thread recv_thread(recv_func);

    // ������Ϣ
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

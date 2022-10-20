#ifndef CHAT_VIEW_H
#define CHAT_VIEW_H

#include <string>
#include <vector>

using namespace std;

class View {
public:
    // ��ʾע�����
    static void show_register_window();

    // ��ʾ������
    static void show_main_window();

    // ��ʾ��ӭ����
    static void show_welcome_window();

    // ��ʾ��Ա����
    static void show_person_info(const vector<string> &usernames);

    // ��ʾ�������
    static void show_chat_window(const string &username);

};

#endif //CHAT_VIEW_H

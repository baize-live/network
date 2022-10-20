#include <vector>
#include <thread>
#include <map>
#include "Model.h"
#include "View.h"
#include "Control.h"
#include "iostream"
#include "../protocol/config.h"
#include "../protocol/protocol.h"

using namespace std;

void Control::handle_welcome(const string &num) {
    if (num == "1") {
        View::show_register_window();
    } else if (num == "2") {
        cout << "��л����ʹ��, �´��ټ�..." << endl;
        system("pause");
        exit(1);
    } else if (num == "3") {
        cout << "2011597 ��ҫ˧" << endl;
        cout << "��Ϣ��ȫרҵ" << endl;
        system("pause");
    } else {
        cout << "����������ȷ������..." << endl;
        system("pause");
    }
}

void Control::handle_register(const string &username) {
    int len = (int) username.size();
    if (len > 10) {
        cout << "��������û�����������... " << endl;
    } else {
        map<string, string> parameters;
        parameters["method"] = Method::method_register;
        parameters["username"] = username;
        Request req(parameters);
        Response resp = req.request(Config::host, Config::port);

        string res = resp.get_res();

        if (res == Res::Success) {
            cout << "ע��ɹ�..." << endl;
            system("pause");
            // Ϊģ�͸�ֵ
            Model::username = username;
            // ��ʾ����
            View::show_main_window();
        } else if (res == Res::Is_not_connected) {
            cout << "�������쳣, ���Ժ�����..." << endl;
        } else if (res == Res::Invalid) {
            cout << "�ǰ���Э�鲻����..." << endl;
        } else {
            cout << "δ֪�쳣, ���Ժ�����..." << endl;
        }
    }
}

void Control::handle_main(const string &num) {
    if (num == "1") {
        map<string, string> parameters;
        parameters["method"] = Method::method_lookup_all_users;
        parameters["username"] = Model::username;
        Request req(parameters);
        Response resp = req.request(Config::host, Config::port);

        string res = resp.get_res();

        if (res == Res::Success) {
            string text = resp.get_text();
            string username;
            vector<string> usernames;
            for (const auto &c: text) {
                if (c == '\r') {
                    break;
                } else if (c == '\n') {
                    usernames.push_back(username);
                    username = "";
                } else {
                    username += c;
                }
            }
            View::show_person_info(usernames);
        } else if (res == Res::Is_not_connected) {
            cout << "�������쳣, ���Ժ�����..." << endl;
        } else if (res == Res::Invalid) {
            cout << "�ǰ���Э�鲻����..." << endl;
        } else {
            cout << "δ֪�쳣, ���Ժ�����..." << endl;
        }
    } else if (num == "2") {
        cout << "�ù�����δ��ͨ" << endl;
        system("pause");
    } else if (num == "3") {
        map<string, string> parameters;
        parameters["method"] = Method::method_recv_info_from_all_user;
        parameters["username"] = Model::username;
        Request req(parameters);
        Response resp = req.request(Config::host, Config::port);

        string res = resp.get_res();

        if (res == Res::Success) {
            string text = resp.get_text();
            if (!text.empty()) {
                string user;
                string time;
                string info;
                int flag = 0;
                for (const auto &c: text) {
                    if (c == '\r') {
                        cout << user << ":" << endl;
                        cout << time << endl;
                        cout << info << endl << endl;
                        time = "";
                        info = "";
                        user = "";
                    } else if (c == '\n') {
                        flag = (flag + 1) % 3;
                    } else {
                        if (flag == 0) {
                            user += c;
                        } else if (flag == 1) {
                            info += c;
                        } else if (flag == 2) {
                            time += c;
                        }
                    }
                }
            }

        } else if (res == Res::Is_not_connected) {
            cout << "�������쳣, ���Ժ�����..." << endl;
        } else if (res == Res::Invalid) {
            cout << "�ǰ���Э�鲻����..." << endl;
        } else {
            cout << "δ֪�쳣, ���Ժ�����..." << endl;
        }
        system("pause");
    } else if (num == "4") {
        cout << "��л����ʹ��, �´��ټ�..." << endl;
        system("pause");
        exit(1);
    } else {
        cout << "����������ȷ������..." << endl;
        system("pause");
    }
}

void Control::handle_person_info(int num, const vector<string> &usernames) {
    if (num > usernames.size() || num <= 0) {
        cout << "����������ȷ������..." << endl;
        system("pause");
    } else {
        // ��������
        Model::user_current_chat = usernames[num - 1];
        View::show_chat_window(Model::username);
    }
}

void Control::recv_user_info_thread() {
    map<string, string> parameters;
    parameters["username_src"] = Model::username;
    parameters["username_des"] = Model::user_current_chat;
    parameters["method"] = Method::method_recv_info_from_user;

    Model::is_loop = true;
    while (Model::is_loop) {
        time_t t = time(nullptr);
        parameters["time"] = ctime(&t);
        Request req(parameters);
        Response resp = req.request(Config::host, Config::port);

        string res = resp.get_res();
        if (res == Res::Success) {
            string text = resp.get_text();
            if (!text.empty()) {
                cout << Model::user_current_chat << ": " << endl;
                string time;
                string info;
                int flag = 0;
                for (const auto &c: text) {
                    if (c == '\r') {
                        cout << time << endl;
                        cout << info << endl << endl;
                        time = "";
                        info = "";
                    } else if (c == '\n') {
                        flag = (flag + 1) % 2;
                    } else {
                        if (flag == 0) {
                            info += c;
                        } else if (flag == 1) {
                            time += c;
                        }
                    }
                }
            }
        } else if (res == Res::Is_not_connected) {
            cout << "�������쳣, ���Ժ�����..." << endl;
        } else if (res == Res::Invalid) {
            cout << "�ǰ���Э�鲻����..." << endl;
        } else {
            cout << "δ֪�쳣, ���Ժ�����..." << endl;
        }

        Sleep(500);
    }
}

void Control::send_user_info_thread(const string &info) {
    map<string, string> parameters;
    parameters["username_src"] = Model::username;
    parameters["username_des"] = Model::user_current_chat;
    time_t t = time(nullptr);
    parameters["time"] = ctime(&t);
    parameters["info"] = info;
    parameters["method"] = Method::method_send_info_to_user;
    Request req(parameters);

    Response resp = req.request(Config::host, Config::port);

    string res = resp.get_res();
    if (res == Res::Success) {
        cout << "\t\t\t" << Model::username << ":" << endl;
        cout << "\t\t\t" << ctime(&t);
        cout << "\t\t\t" << info << endl << endl;
    } else if (res == Res::Is_not_connected) {
        cout << "�������쳣, ���Ժ�����..." << endl;
    } else if (res == Res::Invalid) {
        cout << "�ǰ���Э�鲻����..." << endl;
    } else {
        cout << "δ֪�쳣, ���Ժ�����..." << endl;
    }
}

void Control::stop_loop_recv_info() {
    Model::is_loop = false;
}


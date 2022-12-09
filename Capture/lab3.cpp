#include <iostream>

#include "struct.h"
#include "lab.h"

using namespace std;

int main() {
    // 加载环境
    load_environment();

    // 指向设备链表首部的指针
    pcap_if_t *all_devs;
    int all_devs_num = get_all_dev(all_devs);

    // 拿到想要监听的网卡
    int one_dev_num = 0;
    cout << "请输入要使用的网卡编号:";
    cin >> one_dev_num;
    pcap_if_t *one_dev;
    get_one_dev(one_dev, all_devs, all_devs_num, one_dev_num);
    cout << "网卡信息如下: " << endl;
    ifprint(one_dev);

    // 本机IP 本机MAC
    DWORD localIP = inet_addr(iptos(one_dev->addresses->addr));
    byte localMAC[6] = {};

    // 拿到指定网卡句柄
    pcap_t *ad_handle = get_ad_handle(one_dev, all_devs);

    cout << "获取本机MAC, 请稍等..." << endl;
    if (get_local_mac(ad_handle, localIP, localMAC) == 1) {
        print_ip(localIP);
        print_mac(localMAC);
        cout << endl;
    } else {
        cout << "获取本机MAC失败" << endl;
        exit(1);
    }

    while (true) {
        cout << "请输入要获取MAC的IP地址(输入break 结束):";
        string desIP_str;
        cin >> desIP_str;
        if (desIP_str == "break") {
            break;
        }

        DWORD DesIP = inet_addr(desIP_str.c_str());
        byte DesMAC[6]{};

        if (send_arp(ad_handle, DesIP, localIP, localMAC) == 1 && recv_arp(ad_handle, DesIP, DesMAC) == 1) {
            print_ip(DesIP);
            print_mac(DesMAC);
        } else {
            printf("获取MAC地址失败");
        }

        cout << endl;
    }

    // 关闭指定网卡句柄
    pcap_close(ad_handle);
    WSACleanup();

    system("pause");
    return 0;
}
#include <iostream>

#include "struct.h"
#include "lab.h"

using namespace std;

int main() {
    // ���ػ���
    load_environment();

    // ָ���豸�����ײ���ָ��
    pcap_if_t *all_devs;
    int all_devs_num = get_all_dev(all_devs);

    // �õ���Ҫ����������
    int one_dev_num = 0;
    cout << "������Ҫʹ�õ��������:";
    cin >> one_dev_num;
    pcap_if_t *one_dev;
    get_one_dev(one_dev, all_devs, all_devs_num, one_dev_num);
    cout << "������Ϣ����: " << endl;
    ifprint(one_dev);

    // ����IP ����MAC
    DWORD localIP = inet_addr(iptos(one_dev->addresses->addr));
    byte localMAC[6] = {};

    // �õ�ָ���������
    pcap_t *ad_handle = get_ad_handle(one_dev, all_devs);

    cout << "��ȡ����MAC, ���Ե�..." << endl;
    if (get_local_mac(ad_handle, localIP, localMAC) == 1) {
        print_ip(localIP);
        print_mac(localMAC);
        cout << endl;
    } else {
        cout << "��ȡ����MACʧ��" << endl;
        exit(1);
    }

    while (true) {
        cout << "������Ҫ��ȡMAC��IP��ַ(����break ����):";
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
            printf("��ȡMAC��ַʧ��");
        }

        cout << endl;
    }

    // �ر�ָ���������
    pcap_close(ad_handle);
    WSACleanup();

    system("pause");
    return 0;
}
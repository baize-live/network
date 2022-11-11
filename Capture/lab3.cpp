#include <iostream>

#include "struct.h"
#include "lab.h"

using namespace std;

void send_arp(pcap_t *ad_handle, DWORD DesIP, DWORD SrcIP, byte *SrcMAC) {
    // arp���ṹ��С��42���ֽ�
    const int arp_len = 42;
    byte send_buf[arp_len];

    if (INADDR_NONE != DesIP) {
        set_arp_message(send_buf, arp_len, SrcMAC, SrcIP, DesIP);
        // ������ͳɹ�
        if (pcap_sendpacket(ad_handle, send_buf, arp_len) == 0) {
            printf("ARP Send succeed\n");
        } else {
            printf("ARP Send in getmine Error: %d\n", GetLastError());
        }
    } else {
        printf("\nDesIP ����, PacketSend ʧ��\n");
        exit(1);
    }
}

// �����ֽ���
void capture_arp(pcap_t *&ad_handle, DWORD DesIP) {
    /* Retrieve the packets */
    int res;
    pcap_pkthdr *header;
    const u_char *pkt_data;
    // ��ʼ��ʱ
    DWORD start = GetTickCount();
    while ((res = pcap_next_ex(ad_handle, &header, &pkt_data)) >= 0) {
        if (res == 0) {
            continue;
        }
        if (get_arp_mac((byte *) pkt_data, DesIP) == 0) {
            break;
        }
        if (GetTickCount() - start > 5000) {
            cout << "ARP ��Ӧ��ʱ...." << endl;
            break;
        }
    }

    if (res == -1) {
        cout << "Error reading the packets: " << pcap_geterr(ad_handle) << endl;
        exit(1);
    }
}

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

    // �õ�ָ���������
    pcap_t *ad_handle = get_ad_handle(one_dev, all_devs);

    // ���arp������
    byte SrcMAC[6] = {0x78, 0x2B, 0x46, 0x84, 0x06, 0x11};
    DWORD SrcIP = inet_addr(iptos(one_dev->addresses->addr));

    while (true) {
        cout << "������Ҫ��ȡMAC��IP��ַ(����break ����):";
        string desIP_str;
        cin >> desIP_str;
        if (desIP_str == "break") {
            break;
        }

        DWORD DesIP = inet_addr(desIP_str.c_str());
        // ����arp�����
        send_arp(ad_handle, DesIP, SrcIP, SrcMAC);
        // ����arp��Ӧ��
        capture_arp(ad_handle, DesIP);

        cout << endl;
    }

    // �ر�ָ���������
    pcap_close(ad_handle);
    WSACleanup();

    system("pause");
    return 0;
}
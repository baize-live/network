#include <iostream>

#include "struct.h"
#include "lab.h"

using namespace std;

// ����ARP
void send_arp(pcap_t *ad_handle, DWORD DesIP, DWORD SrcIP, byte *SrcMAC) {
    // arp���ṹ��С��42���ֽ�
    const int arp_len = 42;
    byte send_buf[arp_len];

    if (INADDR_NONE != DesIP) {
        set_arp_message(send_buf, arp_len, SrcMAC, SrcIP, DesIP);
        // ������ͳɹ�
        if (pcap_sendpacket(ad_handle, send_buf, arp_len) == 0) {
            printf("ARP �����ͳɹ�..\n");
        }
    } else {
        printf("\nDesIP ת������, PacketSend ʧ��\n");
        exit(1);
    }
}

// ��ȡ����MAC
void get_local_mac(pcap_t *ad_handle, DWORD localIP, byte *localMAC) {
    printf("���ڻ�ȡ����MAC��ַ, ���Ե�...\n");
    // �򱾻���������arp��
    byte fakeMAC[6] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    DWORD fakeIP = inet_addr("11.11.11.11");
    send_arp(ad_handle, localIP, fakeIP, fakeMAC);

    // ���񱾻���Ӧarp��, ���localMAC
    int res;
    pcap_pkthdr *header;
    const u_char *pkt_data;
    // ��ʼ��ʱ
    DWORD start = GetTickCount();

    while ((res = pcap_next_ex(ad_handle, &header, &pkt_data)) >= 0) {
        if (GetTickCount() - start > 5000) {
            printf("ARP ��Ӧ��ʱ.... ����MAC��ȡʧ��..\n");
            system("pause");
            exit(1);
        }

        if (res == 0) {
            continue;
        }
        auto *mac_frame = (MACFrame_t *) pkt_data;
        if (htons(mac_frame->mac_frame_head.FrameType) != ARP) {
            continue;
        }
        auto *arp_packet = (ArpPacket_t *) mac_frame->data;
        if (htons(arp_packet->op) != ARP_RESPONSE) {
            continue;
        }
        if (arp_packet->SrcIP != localIP) {
            continue;
        }
        memcpy(localMAC, arp_packet->SrcMAC, 6);
        printf("����MAC��ȡ�ɹ�..\n");
        return;
    }
}

// ������Ӧ��ARP
void get_target_mac(pcap_t *ad_handle, DWORD DesIP) {
    int res;
    pcap_pkthdr *header;
    const u_char *pkt_data;
    // ��ʼ��ʱ
    DWORD start = GetTickCount();
    while ((res = pcap_next_ex(ad_handle, &header, &pkt_data)) >= 0) {
        if (GetTickCount() - start > 5000) {
            printf("ARP ��Ӧ��ʱ....\n");
            return;
        }

        if (res == -1) {
            printf("Error reading the packets: %s\n", pcap_geterr(ad_handle));
            system("pause");
            exit(1);
        }

        if (res == 0) {
            continue;
        }

        auto *mac_frame = (MACFrame_t *) pkt_data;
        if (htons(mac_frame->mac_frame_head.FrameType) != ARP) {
            continue;
        }
        auto *arp_packet = (ArpPacket_t *) mac_frame->data;
        if (htons(arp_packet->op) != ARP_RESPONSE || arp_packet->SrcIP != DesIP) {
            continue;
        }
        printf("Ŀ��MAC��ȡ�ɹ�..\n");
        print_ip(DesIP);
        print_mac(arp_packet->SrcMAC);
        return;
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

    // ����IP ����MAC
    DWORD localIP = inet_addr(iptos(one_dev->addresses->addr));
    byte localMAC[6] = {};
    get_local_mac(ad_handle, localIP, localMAC);
    // ��ʾ����IP��MAC
    print_ip(localIP);
    print_mac(localMAC);
    cout << endl;

    while (true) {
        cout << "������Ҫ��ȡMAC��IP��ַ(����break ����):";
        string desIP_str;
        cin >> desIP_str;
        if (desIP_str == "break") {
            break;
        }

        DWORD DesIP = inet_addr(desIP_str.c_str());
        // ����arp�����
        send_arp(ad_handle, DesIP, localIP, localMAC);
        // ����arp��Ӧ��
        get_target_mac(ad_handle, DesIP);

        cout << endl;
    }

    // �ر�ָ���������
    pcap_close(ad_handle);
    WSACleanup();

    system("pause");
    return 0;
}
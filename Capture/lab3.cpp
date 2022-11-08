#include <iostream>

#include "struct.h"
#include "lab.h"

using namespace std;

void send_arp(pcap_t *ad_handle) {
    // arp���ṹ��С��42���ֽ�
    const int arp_len = 42;
    byte send_buf[arp_len];
    // ���arp������
    set_arp_message(send_buf, arp_len);
    // ������ͳɹ�
    if (pcap_sendpacket(ad_handle, send_buf, arp_len) == 0) {
        printf("\nPacketSend succeed\n");
    } else {
        printf("PacketSendPacket in getmine Error: %d\n", GetLastError());
    }
}

int main() {
    // ���ػ���
    load_environment();

    // ָ���豸�����ײ���ָ��
    pcap_if_t *all_devs;
    int all_devs_num = get_all_dev(all_devs);

    // �õ���Ҫ����������
    pcap_if_t *one_dev;
    get_one_dev(one_dev, all_devs, all_devs_num);

    // �õ�ָ���������
    pcap_t *ad_handle = get_ad_handle(one_dev, all_devs);

    // ����arp��
    send_arp(ad_handle);

    // �ر�ָ���������
    pcap_close(ad_handle);
    WSACleanup();

    system("pause");
    return 0;
}
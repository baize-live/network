#include <iostream>

#include "struct.h"
#include "lab.h"

using namespace std;

void send_arp(pcap_t *ad_handle) {
    // arp包结构大小，42个字节
    const int arp_len = 42;
    byte send_buf[arp_len];
    // 填充arp包数据
    set_arp_message(send_buf, arp_len);
    // 如果发送成功
    if (pcap_sendpacket(ad_handle, send_buf, arp_len) == 0) {
        printf("\nPacketSend succeed\n");
    } else {
        printf("PacketSendPacket in getmine Error: %d\n", GetLastError());
    }
}

int main() {
    // 加载环境
    load_environment();

    // 指向设备链表首部的指针
    pcap_if_t *all_devs;
    int all_devs_num = get_all_dev(all_devs);

    // 拿到想要监听的网卡
    pcap_if_t *one_dev;
    get_one_dev(one_dev, all_devs, all_devs_num);

    // 拿到指定网卡句柄
    pcap_t *ad_handle = get_ad_handle(one_dev, all_devs);

    // 发送arp包
    send_arp(ad_handle);

    // 关闭指定网卡句柄
    pcap_close(ad_handle);
    WSACleanup();

    system("pause");
    return 0;
}
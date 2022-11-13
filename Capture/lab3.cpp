#include <iostream>

#include "struct.h"
#include "lab.h"

using namespace std;

// 发送ARP
void send_arp(pcap_t *ad_handle, DWORD DesIP, DWORD SrcIP, byte *SrcMAC) {
    // arp包结构大小，42个字节
    const int arp_len = 42;
    byte send_buf[arp_len];

    if (INADDR_NONE != DesIP) {
        set_arp_message(send_buf, arp_len, SrcMAC, SrcIP, DesIP);
        // 如果发送成功
        if (pcap_sendpacket(ad_handle, send_buf, arp_len) == 0) {
            printf("ARP 请求发送成功..\n");
        }
    } else {
        printf("\nDesIP 转换错误, PacketSend 失败\n");
        exit(1);
    }
}

// 获取本地MAC
void get_local_mac(pcap_t *ad_handle, DWORD localIP, byte *localMAC) {
    printf("正在获取本机MAC地址, 请稍等...\n");
    // 向本机发送请求arp包
    byte fakeMAC[6] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    DWORD fakeIP = inet_addr("11.11.11.11");
    send_arp(ad_handle, localIP, fakeIP, fakeMAC);

    // 捕获本机响应arp包, 获得localMAC
    int res;
    pcap_pkthdr *header;
    const u_char *pkt_data;
    // 开始计时
    DWORD start = GetTickCount();

    while ((res = pcap_next_ex(ad_handle, &header, &pkt_data)) >= 0) {
        if (GetTickCount() - start > 5000) {
            printf("ARP 响应超时.... 本机MAC获取失败..\n");
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
        printf("本机MAC获取成功..\n");
        return;
    }
}

// 捕获响应的ARP
void get_target_mac(pcap_t *ad_handle, DWORD DesIP) {
    int res;
    pcap_pkthdr *header;
    const u_char *pkt_data;
    // 开始计时
    DWORD start = GetTickCount();
    while ((res = pcap_next_ex(ad_handle, &header, &pkt_data)) >= 0) {
        if (GetTickCount() - start > 5000) {
            printf("ARP 响应超时....\n");
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
        printf("目标MAC获取成功..\n");
        print_ip(DesIP);
        print_mac(arp_packet->SrcMAC);
        return;
    }
}

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

    // 拿到指定网卡句柄
    pcap_t *ad_handle = get_ad_handle(one_dev, all_devs);

    // 本机IP 本机MAC
    DWORD localIP = inet_addr(iptos(one_dev->addresses->addr));
    byte localMAC[6] = {};
    get_local_mac(ad_handle, localIP, localMAC);
    // 显示本机IP和MAC
    print_ip(localIP);
    print_mac(localMAC);
    cout << endl;

    while (true) {
        cout << "请输入要获取MAC的IP地址(输入break 结束):";
        string desIP_str;
        cin >> desIP_str;
        if (desIP_str == "break") {
            break;
        }

        DWORD DesIP = inet_addr(desIP_str.c_str());
        // 发送arp请求包
        send_arp(ad_handle, DesIP, localIP, localMAC);
        // 捕获arp响应包
        get_target_mac(ad_handle, DesIP);

        cout << endl;
    }

    // 关闭指定网卡句柄
    pcap_close(ad_handle);
    WSACleanup();

    system("pause");
    return 0;
}
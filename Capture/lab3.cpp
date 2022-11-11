#include <iostream>

#include "struct.h"
#include "lab.h"

using namespace std;

void send_arp(pcap_t *ad_handle, DWORD DesIP, DWORD SrcIP, byte *SrcMAC) {
    // arp包结构大小，42个字节
    const int arp_len = 42;
    byte send_buf[arp_len];

    if (INADDR_NONE != DesIP) {
        set_arp_message(send_buf, arp_len, SrcMAC, SrcIP, DesIP);
        // 如果发送成功
        if (pcap_sendpacket(ad_handle, send_buf, arp_len) == 0) {
            printf("ARP Send succeed\n");
        } else {
            printf("ARP Send in getmine Error: %d\n", GetLastError());
        }
    } else {
        printf("\nDesIP 错误, PacketSend 失败\n");
        exit(1);
    }
}

// 捕获字节流
void capture_arp(pcap_t *&ad_handle, DWORD DesIP) {
    /* Retrieve the packets */
    int res;
    pcap_pkthdr *header;
    const u_char *pkt_data;
    // 开始计时
    DWORD start = GetTickCount();
    while ((res = pcap_next_ex(ad_handle, &header, &pkt_data)) >= 0) {
        if (res == 0) {
            continue;
        }
        if (get_arp_mac((byte *) pkt_data, DesIP) == 0) {
            break;
        }
        if (GetTickCount() - start > 5000) {
            cout << "ARP 响应超时...." << endl;
            break;
        }
    }

    if (res == -1) {
        cout << "Error reading the packets: " << pcap_geterr(ad_handle) << endl;
        exit(1);
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

    // 填充arp包数据
    byte SrcMAC[6] = {0x78, 0x2B, 0x46, 0x84, 0x06, 0x11};
    DWORD SrcIP = inet_addr(iptos(one_dev->addresses->addr));

    while (true) {
        cout << "请输入要获取MAC的IP地址(输入break 结束):";
        string desIP_str;
        cin >> desIP_str;
        if (desIP_str == "break") {
            break;
        }

        DWORD DesIP = inet_addr(desIP_str.c_str());
        // 发送arp请求包
        send_arp(ad_handle, DesIP, SrcIP, SrcMAC);
        // 捕获arp响应包
        capture_arp(ad_handle, DesIP);

        cout << endl;
    }

    // 关闭指定网卡句柄
    pcap_close(ad_handle);
    WSACleanup();

    system("pause");
    return 0;
}
#include <iostream>

#include "struct.h"
#include "lab.h"
#include <vector>
#include <map>

using namespace std;

struct MAC_Address {
    byte MAC[6];
};
struct Routing_Table_Entry {
    DWORD DesIP;
    DWORD Mask;
    DWORD NextIP;
};

DWORD localIP;
byte localMAC[6];
map<DWORD, MAC_Address> ARP_Table;
vector<Routing_Table_Entry> Routing_Table{};

// 捕获字节流
void capture_bytes(pcap_t *&ad_handle);

// 路由转发
void route_forwarding(pcap_t *&ad_handle, byte *buf, int len);

int main() {
    // 加载环境
    load_environment();

    // 指向设备链表首部的指针
    pcap_if_t *all_devs;
    int all_devs_num = get_all_dev(all_devs);

    int one_dev_num = 0;
    cout << "请输入要监听的网卡编号:";
    cin >> one_dev_num;

    // 拿到想要监听的网卡
    pcap_if_t *one_dev;
    get_one_dev(one_dev, all_devs, all_devs_num, one_dev_num);

    cout << "listening on ... 网卡信息如下: " << endl;
    ifprint(one_dev);

    // 拿到指定网卡句柄
    pcap_t *ad_handle = get_ad_handle(one_dev, all_devs);

    // 获取本机IP
    localIP = inet_addr(iptos(one_dev->addresses->addr));
    // 获取本机MAC
    cout << "获取本机MAC, 请稍等..." << endl;
    if (get_local_mac(ad_handle, localIP, localMAC) == 1) {
        print_ip(localIP);
        print_mac(localMAC);
        cout << endl;
    } else {
        cout << "获取本机MAC失败" << endl;
        exit(1);
    }

    // 捕获字节流
    capture_bytes(ad_handle);

    // 关闭指定网卡句柄
    pcap_close(ad_handle);
    WSACleanup();

    system("pause");
    return 0;
}

void capture_bytes(pcap_t *&ad_handle) {
    /* Retrieve the packets */
    int res;
    pcap_pkthdr *header;
    const u_char *pkt_data;
    int pkt_num = 0;

    while ((res = pcap_next_ex(ad_handle, &header, &pkt_data)) >= 0) {

        if (res == 0) {
            continue;
        }

        cout << "第" << ++pkt_num << "条报文 " << endl;

        route_forwarding(ad_handle, (byte *) pkt_data, (int) header->len);
    }
}

void route_forwarding(pcap_t *&ad_handle, byte *buf, int len) {
    auto *MAC_Frame = (MACFrame_t *) buf;

    if (htons(MAC_Frame->MAC_Frame_Head.FrameType) != IPV4) {
        return;
    }

    auto *IPv4_Datagram = (IPV4_Datagram_t *) MAC_Frame->data;

    // 路由转发
    for (auto &item: Routing_Table) {
        // 找到路由表中的目的地址
        if ((IPv4_Datagram->IPV4_Datagram_Head.DesIP & item.Mask) == item.DesIP) {
            // 查找ARP表
            auto it = ARP_Table.find(item.NextIP);
            if (it != ARP_Table.end()) {
                // 找到ARP表中的下一跳地址
                memcpy(MAC_Frame->MAC_Frame_Head.DesMAC, it->second.MAC, 6);
                memcpy(MAC_Frame->MAC_Frame_Head.SrcMAC, localMAC, 6);
            } else {
                // 没有找到ARP表中的下一跳地址
                MAC_Address MAC_address{};
                // 发送ARP请求
                if (send_arp(ad_handle, item.NextIP, localIP, localMAC) == 1
                    && recv_arp(ad_handle, item.NextIP, MAC_address.MAC) == 1) {
                    ARP_Table[item.NextIP] = MAC_address;
                    memcpy(MAC_Frame->MAC_Frame_Head.DesMAC, MAC_address.MAC, 6);
                    memcpy(MAC_Frame->MAC_Frame_Head.SrcMAC, localMAC, 6);
                } else {
                    return;
                }
            }
            pcap_sendpacket(ad_handle, buf, len);
            return;
        }
    }
}

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

MAC_Address localMAC;
Routing_Table_Entry localIP[2];
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

    // 拿到指定网卡句柄
    pcap_t *ad_handle = get_ad_handle(one_dev, all_devs);

    // 获取本机IP
    cout << "获取本机IP, 请稍等..." << endl;
    int i = 0;
    for (pcap_addr_t *addr = one_dev->addresses; addr; addr = addr->next) {
        if (i == 2) {
            break;
        }
        localIP[i].DesIP = inet_addr(iptos(addr->addr));
        localIP[i].Mask = inet_addr(iptos(addr->netmask));
        localIP[i].NextIP = localIP[i].DesIP & localIP[i].Mask;
        cout << "本机IP" << i + 1 << ": ";
        print_ip(localIP[i++].DesIP);
        cout << endl;
    }

    // 获取本机MAC
    cout << "获取本机MAC, 请稍等..." << endl;
    if (get_local_mac(ad_handle, localIP[0].DesIP, localMAC.MAC) == 1) {
        cout << "本机MAC: ";
        print_mac(localMAC.MAC);
        cout << endl;
    } else {
        cout << "获取本机MAC失败" << endl;
        exit(1);
    }

    string str;
    cout << "请添加路由表项.(输入 break 跳出.) " << endl;
    DWORD DesIP, Mask, NextIP;
    while (true) {
        cout << "请输入目的IP地址:";
        cin >> str;
        if (str == "break") {
            break;
        }
        DesIP = inet_addr(str.c_str());
        cout << "请输入子网掩码:";
        cin >> str;
        Mask = inet_addr(str.c_str());
        cout << "请输入下一跳IP地址:";
        cin >> str;
        NextIP = inet_addr(str.c_str());
        Routing_Table.push_back({DesIP, Mask, NextIP});
        cout << endl;
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

MAC_Address find_MAC_from_ARP_Table(pcap_t *&ad_handle, DWORD IP, bool &ret) {
    ret = true;
    if (ARP_Table.find(IP) != ARP_Table.end()) {
        return ARP_Table[IP];
    } else {
        // 发送ARP请求
        MAC_Address MAC_address{};
        if (send_arp(ad_handle, IP, localIP[0].DesIP, localMAC.MAC) == 1
            && recv_arp(ad_handle, IP, MAC_address.MAC) == 1) {
            ARP_Table[IP] = MAC_address;
        } else {
            ret = false;
        }
        return MAC_address;
    }
}

void route_forwarding(pcap_t *&ad_handle, byte *buf, int len) {
    auto *MAC_Frame = (MACFrame_t *) buf;

    if (memcmp(MAC_Frame->MAC_Frame_Head.SrcMAC, localMAC.MAC, 6) == 0) {
        return;
    }

    if (htons(MAC_Frame->MAC_Frame_Head.FrameType) != IPV4) {
        return;
    }

    auto *IPv4_Datagram = (IPV4_Datagram_t *) MAC_Frame->data;

    // 本机IP 不处理
    if (IPv4_Datagram->IPV4_Datagram_Head.DesIP == localIP[0].DesIP ||
        IPv4_Datagram->IPV4_Datagram_Head.DesIP == localIP[1].DesIP) {
        return;
    }

    // 直接投递
    if (((IPv4_Datagram->IPV4_Datagram_Head.DesIP & localIP[0].Mask) == localIP[0].NextIP) ||
        ((IPv4_Datagram->IPV4_Datagram_Head.DesIP & localIP[1].Mask) == localIP[1].NextIP)) {
        bool ret = false;
        MAC_Address MAC_address = find_MAC_from_ARP_Table(ad_handle, IPv4_Datagram->IPV4_Datagram_Head.DesIP, ret);
        if (!ret) {
            cout << "直接投递异常, 获得MAC失败." << endl;
            return;
        }
        memcpy(MAC_Frame->MAC_Frame_Head.DesMAC, MAC_address.MAC, 6);
        memcpy(MAC_Frame->MAC_Frame_Head.SrcMAC, localMAC.MAC, 6);
        cout << "直接投递成功. SrcIP:";
        print_ip(IPv4_Datagram->IPV4_Datagram_Head.SrcIP);
        cout << " DesIP:";
        print_ip(IPv4_Datagram->IPV4_Datagram_Head.DesIP);
        cout << endl;
        pcap_sendpacket(ad_handle, buf, len);
        return;
    }

    // 路由转发
    for (auto &item: Routing_Table) {
        // 找到路由表中的目的地址
        if ((IPv4_Datagram->IPV4_Datagram_Head.DesIP & item.Mask) == item.DesIP) {
            bool ret = false;
            MAC_Address MAC_address = find_MAC_from_ARP_Table(ad_handle, item.NextIP, ret);
            if (!ret) {
                cout << "获得MAC失败, 路由转发异常." << endl;
                return;
            }
            memcpy(MAC_Frame->MAC_Frame_Head.DesMAC, MAC_address.MAC, 6);
            memcpy(MAC_Frame->MAC_Frame_Head.SrcMAC, localMAC.MAC, 6);
            cout << "路由转发成功. SrcIP:";
            print_ip(IPv4_Datagram->IPV4_Datagram_Head.SrcIP);
            cout << " DesIP:";
            print_ip(IPv4_Datagram->IPV4_Datagram_Head.DesIP);
            cout << " NextIP:";
            print_ip(item.NextIP);
            cout << endl;
            pcap_sendpacket(ad_handle, buf, len);
            return;
        }
    }
}

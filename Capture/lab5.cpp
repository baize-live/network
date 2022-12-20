#include <iostream>

#include "struct.h"
#include "lab.h"
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

struct MAC_Address {
    byte MAC[6];
};
struct Routing_Table_Entry {
    DWORD DesIP;
    DWORD Mask;
    DWORD NextIP;
};

int num = 0;
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

    Routing_Table.push_back({localIP[0].DesIP & localIP[0].Mask, localIP[0].Mask, inet_addr("0.0.0.0")});
    Routing_Table.push_back({localIP[1].DesIP & localIP[1].Mask, localIP[1].Mask, inet_addr("0.0.0.0")});
    cout << "添加(add)/删除(del)/显示(print)路由表项.(输入 break 跳出.) " << endl;
    string str;
    DWORD DesIP, Mask, NextIP;
    while (true) {
        cout << "请输入操作: ";
        cin >> str;
        if (str == "break") {
            break;
        } else if (str == "add") {
            cout << "请输入目的IP地址:";
            cin >> str;
            DesIP = inet_addr(str.c_str());
            cout << "请输入子网掩码:";
            cin >> str;
            Mask = inet_addr(str.c_str());
            cout << "请输入下一跳IP地址:";
            cin >> str;
            NextIP = inet_addr(str.c_str());

            for (auto it= Routing_Table.begin(); it != Routing_Table.end(); it++) {
                if (it->DesIP == DesIP && it->Mask == Mask) {
                    it->NextIP = NextIP;
                    cout << "路由表项已存在, 已更新." << endl;
                    break;
                }
                if(it->Mask < Mask) {
                    Routing_Table.insert(it, {DesIP, Mask, NextIP});
                    cout << "路由表项已添加." << endl;
                    break;
                }
                if (it == Routing_Table.end() - 1) {
                    Routing_Table.push_back({DesIP, Mask, NextIP});
                    cout << "路由表项已添加." << endl;
                    break;
                }
            }
        } else if (str == "del") {
            cout << "请输入目的IP地址:";
            cin >> str;
            DesIP = inet_addr(str.c_str());
            cout << "请输入子网掩码:";
            cin >> str;
            Mask = inet_addr(str.c_str());

            if (DesIP == localIP[0].NextIP || DesIP == localIP[1].NextIP) {
                cout << "无法删除默认路由." << endl;
                continue;
            }
            for (auto it = Routing_Table.begin(); it != Routing_Table.end(); it++) {
                if (it->DesIP == DesIP && it->Mask == Mask) {
                    Routing_Table.erase(it);
                    cout << "删除路由成功." << endl;
                    break;
                }
            }
        } else if (str == "print") {
            for (auto &item: Routing_Table) {
                cout << "目的IP: ";
                print_ip(item.DesIP);
                cout << " 子网掩码: ";
                print_ip(item.Mask);
                cout << " 下一跳IP: ";
                print_ip(item.NextIP);
                cout << endl;
            }
        } else {
            cout << "输入错误" << endl;
        }
        cout << endl;
    }

    cout << "开始路由转发..." << endl;

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

    while ((res = pcap_next_ex(ad_handle, &header, &pkt_data)) >= 0) {

        if (res == 0) {
            continue;
        }

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

    // 本机Mac不处理
    if (memcmp(MAC_Frame->MAC_Frame_Head.SrcMAC, localMAC.MAC, 6) == 0) {
        return;
    }

    if (htons(MAC_Frame->MAC_Frame_Head.FrameType) != IPV4) {
        return;
    }

    auto *IPv4_Datagram = (IPV4_Datagram_t *) MAC_Frame->data;

    cout << "第" << ++num << "条IP数据报, ";

    // 本机IP 不处理
    if (IPv4_Datagram->IPV4_Datagram_Head.DesIP == localIP[0].DesIP ||
        IPv4_Datagram->IPV4_Datagram_Head.DesIP == localIP[1].DesIP ||
        (IPv4_Datagram->IPV4_Datagram_Head.DesIP & localIP[0].Mask) ==
        (inet_addr("255.255.255.255") & localIP[0].Mask) ||
        (IPv4_Datagram->IPV4_Datagram_Head.DesIP & localIP[1].Mask) ==
        (inet_addr("255.255.255.255") & localIP[1].Mask)) {
        cout << "目的IP为本机IP, 不处理." << endl;
        return;
    }

    cout << "SrcIP:";
    print_ip(IPv4_Datagram->IPV4_Datagram_Head.SrcIP);
    cout << " DesIP:";
    print_ip(IPv4_Datagram->IPV4_Datagram_Head.DesIP);

    for (auto &item: Routing_Table) {
        // 找到路由表中的目的地址
        if ((IPv4_Datagram->IPV4_Datagram_Head.DesIP & item.Mask) == item.DesIP) {
            bool ret = false;
            MAC_Address MAC_address{};
            if (item.NextIP == 0) {
                // 直接投递
                MAC_address = find_MAC_from_ARP_Table(ad_handle, IPv4_Datagram->IPV4_Datagram_Head.DesIP, ret);
            } else {
                // 路由转发
                MAC_address = find_MAC_from_ARP_Table(ad_handle, item.NextIP, ret);
            }
            if (!ret) {
                cout << " 获得MAC失败" << endl;
                return;
            }
            memcpy(MAC_Frame->MAC_Frame_Head.DesMAC, MAC_address.MAC, 6);
            memcpy(MAC_Frame->MAC_Frame_Head.SrcMAC, localMAC.MAC, 6);
            cout << (item.NextIP == 0 ? " 直接投递. NextIP:" : " 转发成功. NextIP:");
            print_ip(item.NextIP == 0 ? IPv4_Datagram->IPV4_Datagram_Head.DesIP : item.NextIP);
            cout << endl;
            pcap_sendpacket(ad_handle, buf, len);
            return;
        }
    }

    cout << " 转发失败." << endl;
}

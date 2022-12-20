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

// �����ֽ���
void capture_bytes(pcap_t *&ad_handle);

// ·��ת��
void route_forwarding(pcap_t *&ad_handle, byte *buf, int len);

int main() {
    // ���ػ���
    load_environment();

    // ָ���豸�����ײ���ָ��
    pcap_if_t *all_devs;
    int all_devs_num = get_all_dev(all_devs);

    int one_dev_num = 0;
    cout << "������Ҫ�������������:";
    cin >> one_dev_num;

    // �õ���Ҫ����������
    pcap_if_t *one_dev;
    get_one_dev(one_dev, all_devs, all_devs_num, one_dev_num);

    // �õ�ָ���������
    pcap_t *ad_handle = get_ad_handle(one_dev, all_devs);

    // ��ȡ����IP
    cout << "��ȡ����IP, ���Ե�..." << endl;
    int i = 0;
    for (pcap_addr_t *addr = one_dev->addresses; addr; addr = addr->next) {
        if (i == 2) {
            break;
        }
        localIP[i].DesIP = inet_addr(iptos(addr->addr));
        localIP[i].Mask = inet_addr(iptos(addr->netmask));
        localIP[i].NextIP = localIP[i].DesIP & localIP[i].Mask;
        cout << "����IP" << i + 1 << ": ";
        print_ip(localIP[i++].DesIP);
        cout << endl;
    }

    // ��ȡ����MAC
    cout << "��ȡ����MAC, ���Ե�..." << endl;
    if (get_local_mac(ad_handle, localIP[0].DesIP, localMAC.MAC) == 1) {
        cout << "����MAC: ";
        print_mac(localMAC.MAC);
        cout << endl;
    } else {
        cout << "��ȡ����MACʧ��" << endl;
        exit(1);
    }

    Routing_Table.push_back({localIP[0].DesIP & localIP[0].Mask, localIP[0].Mask, inet_addr("0.0.0.0")});
    Routing_Table.push_back({localIP[1].DesIP & localIP[1].Mask, localIP[1].Mask, inet_addr("0.0.0.0")});
    cout << "���(add)/ɾ��(del)/��ʾ(print)·�ɱ���.(���� break ����.) " << endl;
    string str;
    DWORD DesIP, Mask, NextIP;
    while (true) {
        cout << "���������: ";
        cin >> str;
        if (str == "break") {
            break;
        } else if (str == "add") {
            cout << "������Ŀ��IP��ַ:";
            cin >> str;
            DesIP = inet_addr(str.c_str());
            cout << "��������������:";
            cin >> str;
            Mask = inet_addr(str.c_str());
            cout << "��������һ��IP��ַ:";
            cin >> str;
            NextIP = inet_addr(str.c_str());

            for (auto it= Routing_Table.begin(); it != Routing_Table.end(); it++) {
                if (it->DesIP == DesIP && it->Mask == Mask) {
                    it->NextIP = NextIP;
                    cout << "·�ɱ����Ѵ���, �Ѹ���." << endl;
                    break;
                }
                if(it->Mask < Mask) {
                    Routing_Table.insert(it, {DesIP, Mask, NextIP});
                    cout << "·�ɱ��������." << endl;
                    break;
                }
                if (it == Routing_Table.end() - 1) {
                    Routing_Table.push_back({DesIP, Mask, NextIP});
                    cout << "·�ɱ��������." << endl;
                    break;
                }
            }
        } else if (str == "del") {
            cout << "������Ŀ��IP��ַ:";
            cin >> str;
            DesIP = inet_addr(str.c_str());
            cout << "��������������:";
            cin >> str;
            Mask = inet_addr(str.c_str());

            if (DesIP == localIP[0].NextIP || DesIP == localIP[1].NextIP) {
                cout << "�޷�ɾ��Ĭ��·��." << endl;
                continue;
            }
            for (auto it = Routing_Table.begin(); it != Routing_Table.end(); it++) {
                if (it->DesIP == DesIP && it->Mask == Mask) {
                    Routing_Table.erase(it);
                    cout << "ɾ��·�ɳɹ�." << endl;
                    break;
                }
            }
        } else if (str == "print") {
            for (auto &item: Routing_Table) {
                cout << "Ŀ��IP: ";
                print_ip(item.DesIP);
                cout << " ��������: ";
                print_ip(item.Mask);
                cout << " ��һ��IP: ";
                print_ip(item.NextIP);
                cout << endl;
            }
        } else {
            cout << "�������" << endl;
        }
        cout << endl;
    }

    cout << "��ʼ·��ת��..." << endl;

    // �����ֽ���
    capture_bytes(ad_handle);

    // �ر�ָ���������
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
        // ����ARP����
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

    // ����Mac������
    if (memcmp(MAC_Frame->MAC_Frame_Head.SrcMAC, localMAC.MAC, 6) == 0) {
        return;
    }

    if (htons(MAC_Frame->MAC_Frame_Head.FrameType) != IPV4) {
        return;
    }

    auto *IPv4_Datagram = (IPV4_Datagram_t *) MAC_Frame->data;

    cout << "��" << ++num << "��IP���ݱ�, ";

    // ����IP ������
    if (IPv4_Datagram->IPV4_Datagram_Head.DesIP == localIP[0].DesIP ||
        IPv4_Datagram->IPV4_Datagram_Head.DesIP == localIP[1].DesIP ||
        (IPv4_Datagram->IPV4_Datagram_Head.DesIP & localIP[0].Mask) ==
        (inet_addr("255.255.255.255") & localIP[0].Mask) ||
        (IPv4_Datagram->IPV4_Datagram_Head.DesIP & localIP[1].Mask) ==
        (inet_addr("255.255.255.255") & localIP[1].Mask)) {
        cout << "Ŀ��IPΪ����IP, ������." << endl;
        return;
    }

    cout << "SrcIP:";
    print_ip(IPv4_Datagram->IPV4_Datagram_Head.SrcIP);
    cout << " DesIP:";
    print_ip(IPv4_Datagram->IPV4_Datagram_Head.DesIP);

    for (auto &item: Routing_Table) {
        // �ҵ�·�ɱ��е�Ŀ�ĵ�ַ
        if ((IPv4_Datagram->IPV4_Datagram_Head.DesIP & item.Mask) == item.DesIP) {
            bool ret = false;
            MAC_Address MAC_address{};
            if (item.NextIP == 0) {
                // ֱ��Ͷ��
                MAC_address = find_MAC_from_ARP_Table(ad_handle, IPv4_Datagram->IPV4_Datagram_Head.DesIP, ret);
            } else {
                // ·��ת��
                MAC_address = find_MAC_from_ARP_Table(ad_handle, item.NextIP, ret);
            }
            if (!ret) {
                cout << " ���MACʧ��" << endl;
                return;
            }
            memcpy(MAC_Frame->MAC_Frame_Head.DesMAC, MAC_address.MAC, 6);
            memcpy(MAC_Frame->MAC_Frame_Head.SrcMAC, localMAC.MAC, 6);
            cout << (item.NextIP == 0 ? " ֱ��Ͷ��. NextIP:" : " ת���ɹ�. NextIP:");
            print_ip(item.NextIP == 0 ? IPv4_Datagram->IPV4_Datagram_Head.DesIP : item.NextIP);
            cout << endl;
            pcap_sendpacket(ad_handle, buf, len);
            return;
        }
    }

    cout << " ת��ʧ��." << endl;
}

#include <iostream>
#include "struct.h"

using namespace std;

// ����IP���ݱ�
void parse_ip(IPV4_Datagram_t *ip_datagram) {
    cout << "��ʼ����IP���ݱ�..." << endl;
    cout << "\tԴ��IP: ";
    print_ip(ip_datagram->IPV4_Datagram_Head.SrcIP);
    cout << "\tĿ��IP: ";
    print_ip(ip_datagram->IPV4_Datagram_Head.DesIP);
    cout << "\t���ĳ���: " << ip_datagram->IPV4_Datagram_Head.TotalLen << endl;
    cout << "\t��������: " << endl;
    switch (htons(ip_datagram->IPV4_Datagram_Head.Protocol)) {
        case ICMP: {
            cout << "ICMPЭ��";
            break;
        }
        case TCP: {
            cout << "TCPЭ��";
            break;
        }
        case UDP: {
            cout << "UDPЭ��";
            break;
        }
        default:
            cout << "����Э��" << endl;
            break;
    }
    cout << "��ʱ�����������Э��" << endl;
}

// ����ARP���ݰ�
void parse_arp(ArpPacket_t *arp_packet) {
    cout << "��ʼ����ARP���ݰ�...";
    switch (htons(arp_packet->op)) {
        case ARP_REQUEST:
            cout << "ARP Request" << endl;
            break;
        case ARP_RESPONSE:
            cout << "ARP Response" << endl;
            break;
    }
}

// ����mac֡
void parse_mac(MACFrame_t *mac_frame) {
    cout << "��ʼ����MAC֡..." << endl;
    cout << "\tԴ��MAC: ";
    print_mac(mac_frame->MAC_Frame_Head.SrcMAC);
    cout << "\tĿ��MAC: ";
    print_mac(mac_frame->MAC_Frame_Head.DesMAC);
    cout << "\tMAC֡����: ";

    switch (htons(mac_frame->MAC_Frame_Head.FrameType)) {
        case IPV4: {
            cout << "IPV4Э��" << endl;
            auto *ip_datagram = (IPV4_Datagram_t *) mac_frame->data;
            parse_ip(ip_datagram);
            break;
        }
        case IPV6: {
            cout << "IPV6Э��(��ǰֻ����IPV4Э��)" << endl;
            break;
        }
        case ARP: {
            cout << "ARPЭ��" << endl;
            auto *arp_packet = (ArpPacket_t *) mac_frame->data;
            parse_arp(arp_packet);
            break;
        }
        default:
            cout << "����Э��(��ǰֻ����IPV4Э��)" << endl;
            break;
    }
}

// ��������
void parse_message(byte *buf) {
    auto *mac_frame = (MACFrame_t *) buf;
    parse_mac(mac_frame);
}

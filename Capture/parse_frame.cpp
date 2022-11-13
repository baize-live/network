#include <iostream>
#include "struct.h"

using namespace std;

// ����IP���ݱ�
void parse_ip(IPV4_Datagram_t *ip_datagram) {
    cout << "��ʼ����IP���ݱ�...";
    cout << endl << "\tԴ��IP: ";
    print_ip(ip_datagram->ipv4_datagram_head.SrcIP);
    cout << endl << "\tĿ��IP: ";
    print_ip(ip_datagram->ipv4_datagram_head.DesIP);
    cout << endl << "\t���ĳ���: " << ip_datagram->ipv4_datagram_head.TotalLen;
    cout << endl << "\t��������: ";
    switch (htons(ip_datagram->ipv4_datagram_head.Protocol)) {
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
    cout << " ��ʱ�����������Э��" << endl;
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
    cout << "��ʼ����MAC֡...";
    cout << endl << "\tԴ��MAC: ";
    print_mac(mac_frame->mac_frame_head.SrcMAC);
    cout << endl << "\tĿ��MAC: ";
    print_mac(mac_frame->mac_frame_head.DesMAC);
    cout << endl << "\tMAC֡����: ";

    switch (htons(mac_frame->mac_frame_head.FrameType)) {
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

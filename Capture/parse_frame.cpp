#include <iostream>
#include "struct.h"

using namespace std;

// ����ֽ��� 0x68 0x00
void print_bytes(byte *buf, const int len) {
    for (int i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
    }
}

// ���IP 192.168.1.1
void print_ip(DWORD IP_addr) {
    printf("%lu", IP_addr & 0xFF);
    printf(".%lu", (IP_addr >> 8) & 0xFF);
    printf(".%lu", (IP_addr >> 16) & 0xFF);
    printf(".%lu", (IP_addr >> 24) & 0xFF);
}

// ���MAC
void print_mac(byte *mac) {
    printf("%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

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
    print_bytes(mac_frame->mac_frame_head.SrcMAC, sizeof(mac_frame->mac_frame_head.SrcMAC));
    cout << endl << "\tĿ��MAC: ";
    print_bytes(mac_frame->mac_frame_head.DesMAC, sizeof(mac_frame->mac_frame_head.DesMAC));
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

int get_arp_mac(byte *buf, DWORD DesIP) {
    auto *mac_frame = (MACFrame_t *) buf;
    if (htons(mac_frame->mac_frame_head.FrameType) != ARP) {
        return 1;
    }
    auto *arp_packet = (ArpPacket_t *) mac_frame->data;
    if (htons(arp_packet->op) != ARP_RESPONSE) {
        return 2;
    }
    if (arp_packet->SrcIP != DesIP) {
        return 3;
    }
    printf("ARP res recv succeed ");
    cout << "IP: ";
    print_ip(DesIP);
    cout << " MAC: ";
    print_mac(arp_packet->SrcMAC);
    cout << endl;
    return 0;
}
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
void print_ip(byte *IP_addr) {
    printf("%d", IP_addr[0]);
    for (int i = 1; i < 4; ++i) {
        printf(".%d", IP_addr[i]);
    }
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
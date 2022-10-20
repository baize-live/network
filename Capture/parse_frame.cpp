#include <iostream>
#include "parse_frame.h"
using namespace std;

// ����ֽ���
void print_bytes(const byte *buf, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
    }
}

// ����IP���ݱ�
void parse_ip(IPDatagram_t *ip_datagram) {
    auto print_ip = [](ULONG IP_addr) {
        unsigned char *ip = (unsigned char *) &IP_addr;
        printf("%d", ip[0]);
        for (int i = 1; i < 4; ++i) {
            printf(".%d", ip[i]);
        }
    };

    cout << "��ʼ����IP���ݱ�...";
    cout << endl << "\tԴ��IP: ";
    print_ip(ip_datagram->SrcIP);
    cout << endl << "\tĿ��IP: ";
    print_ip(ip_datagram->DstIP);
    cout << endl << "\t���ĳ���: " << ip_datagram->TotalLen;
    cout << endl << "\t��������: ";
    switch (ip_datagram->Protocol) {
        case 1: {
            cout << "ICMPЭ��";
            break;
        }
        case 6: {
            cout << "TCPЭ��";
            break;
        }
        case 17: {
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
    print_bytes(mac_frame->SrcMAC, sizeof(mac_frame->SrcMAC));
    cout << endl << "\tĿ��MAC: ";
    print_bytes(mac_frame->DesMAC, sizeof(mac_frame->DesMAC));
    cout << endl << "\tMAC֡����: ";

    switch (mac_frame->FrameType) {
        case 0x0008: {
            cout << "IPV4Э��" << endl;
            auto *ip_datagram = (IPDatagram_t *) mac_frame->data;
            parse_ip(ip_datagram);
            break;
        }
        case 0xdd86: {
            cout << "IPV6Э��(��ǰֻ����IPV4Э��)" << endl;
//            auto *ip_datagram = (IPDatagram_t *) mac_frame->data;
//            parse_ip(ip_datagram);
            break;
        }
        default:
            cout << "����Э��(��ǰֻ����IPV4Э��)" << endl;
            break;
    }
}

// ��������
void parse_message(const byte *buf) {

    auto *mac_frame = (MACFrame_t *) buf;
    parse_mac(mac_frame);

}
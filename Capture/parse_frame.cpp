#include <iostream>
#include "parse_frame.h"
using namespace std;

// 输出字节流
void print_bytes(const byte *buf, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
    }
}

// 解析IP数据报
void parse_ip(IPDatagram_t *ip_datagram) {
    auto print_ip = [](ULONG IP_addr) {
        unsigned char *ip = (unsigned char *) &IP_addr;
        printf("%d", ip[0]);
        for (int i = 1; i < 4; ++i) {
            printf(".%d", ip[i]);
        }
    };

    cout << "开始解析IP数据报...";
    cout << endl << "\t源发IP: ";
    print_ip(ip_datagram->SrcIP);
    cout << endl << "\t目的IP: ";
    print_ip(ip_datagram->DstIP);
    cout << endl << "\t报文长度: " << ip_datagram->TotalLen;
    cout << endl << "\t报文类型: ";
    switch (ip_datagram->Protocol) {
        case 1: {
            cout << "ICMP协议";
            break;
        }
        case 6: {
            cout << "TCP协议";
            break;
        }
        case 17: {
            cout << "UDP协议";
            break;
        }
        default:
            cout << "其他协议" << endl;
            break;
    }
    cout << " 暂时不解析运输层协议" << endl;
}

// 解析mac帧
void parse_mac(MACFrame_t *mac_frame) {
    cout << "开始解析MAC帧...";
    cout << endl << "\t源发MAC: ";
    print_bytes(mac_frame->SrcMAC, sizeof(mac_frame->SrcMAC));
    cout << endl << "\t目的MAC: ";
    print_bytes(mac_frame->DesMAC, sizeof(mac_frame->DesMAC));
    cout << endl << "\tMAC帧类型: ";

    switch (mac_frame->FrameType) {
        case 0x0008: {
            cout << "IPV4协议" << endl;
            auto *ip_datagram = (IPDatagram_t *) mac_frame->data;
            parse_ip(ip_datagram);
            break;
        }
        case 0xdd86: {
            cout << "IPV6协议(当前只解析IPV4协议)" << endl;
//            auto *ip_datagram = (IPDatagram_t *) mac_frame->data;
//            parse_ip(ip_datagram);
            break;
        }
        default:
            cout << "其他协议(当前只解析IPV4协议)" << endl;
            break;
    }
}

// 解析报文
void parse_message(const byte *buf) {

    auto *mac_frame = (MACFrame_t *) buf;
    parse_mac(mac_frame);

}
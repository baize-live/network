#include <iostream>
#include "struct.h"

using namespace std;

// 输出字节流 0x68 0x00
void print_bytes(byte *buf, const int len) {
    for (int i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
    }
}

// 输出IP 192.168.1.1
void print_ip(byte *IP_addr) {
    printf("%d", IP_addr[0]);
    for (int i = 1; i < 4; ++i) {
        printf(".%d", IP_addr[i]);
    }
}

// 解析IP数据报
void parse_ip(IPV4_Datagram_t *ip_datagram) {
    cout << "开始解析IP数据报...";
    cout << endl << "\t源发IP: ";
    print_ip(ip_datagram->ipv4_datagram_head.SrcIP);
    cout << endl << "\t目的IP: ";
    print_ip(ip_datagram->ipv4_datagram_head.DesIP);
    cout << endl << "\t报文长度: " << ip_datagram->ipv4_datagram_head.TotalLen;
    cout << endl << "\t报文类型: ";
    switch (htons(ip_datagram->ipv4_datagram_head.Protocol)) {
        case ICMP: {
            cout << "ICMP协议";
            break;
        }
        case TCP: {
            cout << "TCP协议";
            break;
        }
        case UDP: {
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
    print_bytes(mac_frame->mac_frame_head.SrcMAC, sizeof(mac_frame->mac_frame_head.SrcMAC));
    cout << endl << "\t目的MAC: ";
    print_bytes(mac_frame->mac_frame_head.DesMAC, sizeof(mac_frame->mac_frame_head.DesMAC));
    cout << endl << "\tMAC帧类型: ";

    switch (htons(mac_frame->mac_frame_head.FrameType)) {
        case IPV4: {
            cout << "IPV4协议" << endl;
            auto *ip_datagram = (IPV4_Datagram_t *) mac_frame->data;
            parse_ip(ip_datagram);
            break;
        }
        case IPV6: {
            cout << "IPV6协议(当前只解析IPV4协议)" << endl;
            break;
        }
        default:
            cout << "其他协议(当前只解析IPV4协议)" << endl;
            break;
    }
}

// 解析报文
void parse_message(byte *buf) {
    auto *mac_frame = (MACFrame_t *) buf;
    parse_mac(mac_frame);
}
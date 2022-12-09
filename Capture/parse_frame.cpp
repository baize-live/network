#include <iostream>
#include "struct.h"

using namespace std;

// 解析IP数据报
void parse_ip(IPV4_Datagram_t *ip_datagram) {
    cout << "开始解析IP数据报..." << endl;
    cout << "\t源发IP: ";
    print_ip(ip_datagram->IPV4_Datagram_Head.SrcIP);
    cout << "\t目的IP: ";
    print_ip(ip_datagram->IPV4_Datagram_Head.DesIP);
    cout << "\t报文长度: " << ip_datagram->IPV4_Datagram_Head.TotalLen << endl;
    cout << "\t报文类型: " << endl;
    switch (htons(ip_datagram->IPV4_Datagram_Head.Protocol)) {
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
    cout << "暂时不解析运输层协议" << endl;
}

// 解析ARP数据包
void parse_arp(ArpPacket_t *arp_packet) {
    cout << "开始解析ARP数据包...";
    switch (htons(arp_packet->op)) {
        case ARP_REQUEST:
            cout << "ARP Request" << endl;
            break;
        case ARP_RESPONSE:
            cout << "ARP Response" << endl;
            break;
    }
}

// 解析mac帧
void parse_mac(MACFrame_t *mac_frame) {
    cout << "开始解析MAC帧..." << endl;
    cout << "\t源发MAC: ";
    print_mac(mac_frame->MAC_Frame_Head.SrcMAC);
    cout << "\t目的MAC: ";
    print_mac(mac_frame->MAC_Frame_Head.DesMAC);
    cout << "\tMAC帧类型: ";

    switch (htons(mac_frame->MAC_Frame_Head.FrameType)) {
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
        case ARP: {
            cout << "ARP协议" << endl;
            auto *arp_packet = (ArpPacket_t *) mac_frame->data;
            parse_arp(arp_packet);
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

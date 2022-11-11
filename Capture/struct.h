#ifndef CAPTURE_STRUCT_H
#define CAPTURE_STRUCT_H

#include "pcap.h"

#pragma pack(1)        // 进入字节对齐方式

// MAC帧头 14Bytes
struct MACFrame_Head_t {
    BYTE DesMAC[6];    // 目的MAC地址
    BYTE SrcMAC[6];    // 源MAC地址
    WORD FrameType;    // 帧类型
};

// IPV4数据报头 40Bytes
struct IPV4_Datagram_Head_t {
    BYTE Ver_HLen;
    BYTE TOS;
    WORD TotalLen;      // 数据报总长度
    WORD ID;
    WORD Flag_Segment;
    BYTE TTL;
    BYTE Protocol;      // 上层协议
    WORD Checksum;
    DWORD SrcIP;        // 源IP地址
    DWORD DesIP;      // 目的IP地址
};

// MAC帧 1514Bytes
struct MACFrame_t {
    MACFrame_Head_t mac_frame_head; // MAC帧头
    BYTE data[1500];                // 剩余数据
};

// IPV4数据报 1500Bytes
struct IPV4_Datagram_t {
    IPV4_Datagram_Head_t ipv4_datagram_head; // IP数据报头
    BYTE data[1460];                    // 剩余数据
};

// ARP数据包 28字节
struct ArpPacket_t {
    WORD Hardware_Type;         // 硬件类型
    WORD Protocol_Type;         // 协议类型
    BYTE Hardware_Addr_Size;    // 硬件地址长度
    BYTE Protocol_Addr_Size;    // 协议地址长度
    WORD op;                    // 操作类型
    BYTE SrcMAC[6];             // 源MAC地址
    DWORD SrcIP;                // 源IP地址
    BYTE DesMAC[6];             // 目的MAC地址
    DWORD DesIP;                // 目的IP地址
};

#pragma pack()

// 帧类型枚举
enum FrameType {
    IPV4 = 0x0800,
    IPV6 = 0x86dd,
    ARP = 0x0806,
};

// 协议枚举
enum Protocol {
    ICMP = 0x01,
    TCP = 0x06,
    UDP = 0x11,
};

// ARP的OP枚举
enum ARP_OP {
    ARP_REQUEST = 1,            // ARP请求
    ARP_RESPONSE = 2,           // ARP应答
};

enum ARP_Hardware_Type {
    ARP_HARDWARE = 1,       //硬件类型字段值为表示以太网地址
};

enum ARP_Protocol_Type {
    ETH_IP = 0x0800,
};

// 输出字节流
void print_bytes(byte *buf, int len);

// 解析报文
void parse_message(byte *buf);

// 拿到IP和MAC映射
int get_arp_mac(byte *buf, DWORD DesIP);

// ARP数据包
void set_arp_message(byte *buf, int len, byte *SrcMAC, DWORD SrcIP, DWORD DesIP);

#endif //CAPTURE_STRUCT_H

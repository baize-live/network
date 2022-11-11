#ifndef CAPTURE_STRUCT_H
#define CAPTURE_STRUCT_H

#include "pcap.h"

#pragma pack(1)        // �����ֽڶ��뷽ʽ

// MAC֡ͷ 14Bytes
struct MACFrame_Head_t {
    BYTE DesMAC[6];    // Ŀ��MAC��ַ
    BYTE SrcMAC[6];    // ԴMAC��ַ
    WORD FrameType;    // ֡����
};

// IPV4���ݱ�ͷ 40Bytes
struct IPV4_Datagram_Head_t {
    BYTE Ver_HLen;
    BYTE TOS;
    WORD TotalLen;      // ���ݱ��ܳ���
    WORD ID;
    WORD Flag_Segment;
    BYTE TTL;
    BYTE Protocol;      // �ϲ�Э��
    WORD Checksum;
    DWORD SrcIP;        // ԴIP��ַ
    DWORD DesIP;      // Ŀ��IP��ַ
};

// MAC֡ 1514Bytes
struct MACFrame_t {
    MACFrame_Head_t mac_frame_head; // MAC֡ͷ
    BYTE data[1500];                // ʣ������
};

// IPV4���ݱ� 1500Bytes
struct IPV4_Datagram_t {
    IPV4_Datagram_Head_t ipv4_datagram_head; // IP���ݱ�ͷ
    BYTE data[1460];                    // ʣ������
};

// ARP���ݰ� 28�ֽ�
struct ArpPacket_t {
    WORD Hardware_Type;         // Ӳ������
    WORD Protocol_Type;         // Э������
    BYTE Hardware_Addr_Size;    // Ӳ����ַ����
    BYTE Protocol_Addr_Size;    // Э���ַ����
    WORD op;                    // ��������
    BYTE SrcMAC[6];             // ԴMAC��ַ
    DWORD SrcIP;                // ԴIP��ַ
    BYTE DesMAC[6];             // Ŀ��MAC��ַ
    DWORD DesIP;                // Ŀ��IP��ַ
};

#pragma pack()

// ֡����ö��
enum FrameType {
    IPV4 = 0x0800,
    IPV6 = 0x86dd,
    ARP = 0x0806,
};

// Э��ö��
enum Protocol {
    ICMP = 0x01,
    TCP = 0x06,
    UDP = 0x11,
};

// ARP��OPö��
enum ARP_OP {
    ARP_REQUEST = 1,            // ARP����
    ARP_RESPONSE = 2,           // ARPӦ��
};

enum ARP_Hardware_Type {
    ARP_HARDWARE = 1,       //Ӳ�������ֶ�ֵΪ��ʾ��̫����ַ
};

enum ARP_Protocol_Type {
    ETH_IP = 0x0800,
};

// ����ֽ���
void print_bytes(byte *buf, int len);

// ��������
void parse_message(byte *buf);

// �õ�IP��MACӳ��
int get_arp_mac(byte *buf, DWORD DesIP);

// ARP���ݰ�
void set_arp_message(byte *buf, int len, byte *SrcMAC, DWORD SrcIP, DWORD DesIP);

#endif //CAPTURE_STRUCT_H

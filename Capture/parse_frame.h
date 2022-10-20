#ifndef CAPTURE_PARSE_FRAME_H
#define CAPTURE_PARSE_FRAME_H
#include "pcap.h"

#pragma pack(1)        //�����ֽڶ��뷽ʽ

//MAC֡ 1514Bytes
typedef struct MACFrame_t {
    BYTE DesMAC[6];    // Ŀ�ĵ�ַ
    BYTE SrcMAC[6];    // Դ��ַ
    WORD FrameType;    // ֡����
    BYTE data[1500];   // ʣ������
} MACFrame_t;

//IP���ݱ� 1500Bytes
typedef struct IPDatagram_t {
    BYTE Ver_HLen;
    BYTE TOS;
    WORD TotalLen;
    WORD ID;
    WORD Flag_Segment;
    BYTE TTL;
    BYTE Protocol;
    WORD Checksum;
    ULONG SrcIP;
    ULONG DstIP;
    BYTE data[1460];   // ʣ������
} IPDatagram_t;

// ����ֽ���
void print_bytes(const byte *buf, int len);

// ��������
void parse_message(const byte *buf);
#endif //CAPTURE_PARSE_FRAME_H

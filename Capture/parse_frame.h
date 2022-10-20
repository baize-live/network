#ifndef CAPTURE_PARSE_FRAME_H
#define CAPTURE_PARSE_FRAME_H
#include "pcap.h"

#pragma pack(1)        //进入字节对齐方式

//MAC帧 1514Bytes
typedef struct MACFrame_t {
    BYTE DesMAC[6];    // 目的地址
    BYTE SrcMAC[6];    // 源地址
    WORD FrameType;    // 帧类型
    BYTE data[1500];   // 剩余数据
} MACFrame_t;

//IP数据报 1500Bytes
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
    BYTE data[1460];   // 剩余数据
} IPDatagram_t;

// 输出字节流
void print_bytes(const byte *buf, int len);

// 解析报文
void parse_message(const byte *buf);
#endif //CAPTURE_PARSE_FRAME_H

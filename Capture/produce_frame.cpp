#include "struct.h"

MACFrame_Head_t produce_mac_frame_Head(byte *DesMAC, byte *SrcMAC, WORD FrameType) {
    MACFrame_Head_t mac_frame_head{};

    MACFrame_Head_t *mac_frame_head_ptr = &mac_frame_head;
    memcpy(mac_frame_head_ptr->DesMAC, DesMAC, 6);
    memcpy(mac_frame_head_ptr->SrcMAC, SrcMAC, 6);
    mac_frame_head.FrameType = htons(FrameType);

    return mac_frame_head;
}

ArpPacket_t produce_arp_frame(WORD Hardware_Type, WORD Protocol_Type,
                              WORD op, byte *SrcMAC, DWORD SrcIP, DWORD DesIP) {
    ArpPacket_t arp_frame{};

    memcpy(arp_frame.SrcMAC, SrcMAC, 6);        //ARP字段源MAC地址
    arp_frame.SrcIP = SrcIP;          //ARP字段源IP地址
    memset(arp_frame.DesMAC, 0x00, 6);          //ARP字段目的MAC地址
    arp_frame.DesIP = DesIP;          //ARP字段目的IP地址
    arp_frame.Hardware_Type = htons(Hardware_Type); // 硬件类型
    arp_frame.Protocol_Type = htons(Protocol_Type); // 协议类型
    arp_frame.Hardware_Addr_Size = 6;   // 硬件地址长度
    arp_frame.Protocol_Addr_Size = 4;   // 协议地址长度
    arp_frame.op = htons(op);   // 操作类型

    return arp_frame;
}

void set_arp_message(byte *buf, int len, byte *SrcMAC, DWORD SrcIP, DWORD DesIP) {
    // 生成MAC帧头
    byte DesMAC[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    MACFrame_Head_t mac_frame_head = produce_mac_frame_Head(DesMAC, SrcMAC, ARP);

    // ARP数据包
    ArpPacket_t arp_frame = produce_arp_frame(ARP_HARDWARE, ETH_IP, ARP_REQUEST, SrcMAC, SrcIP, DesIP);

    // 构造一个ARP请求
    memset(buf, 0, len);   //ARP清零
    memcpy(buf, &mac_frame_head, sizeof(MACFrame_Head_t));
    memcpy(buf + sizeof(MACFrame_Head_t), &arp_frame, sizeof(ArpPacket_t));
}

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

    memcpy(arp_frame.SrcMAC, SrcMAC, 6);        //ARP�ֶ�ԴMAC��ַ
    arp_frame.SrcIP = SrcIP;          //ARP�ֶ�ԴIP��ַ
    memset(arp_frame.DesMAC, 0x00, 6);          //ARP�ֶ�Ŀ��MAC��ַ
    arp_frame.DesIP = DesIP;          //ARP�ֶ�Ŀ��IP��ַ
    arp_frame.Hardware_Type = htons(Hardware_Type); // Ӳ������
    arp_frame.Protocol_Type = htons(Protocol_Type); // Э������
    arp_frame.Hardware_Addr_Size = 6;   // Ӳ����ַ����
    arp_frame.Protocol_Addr_Size = 4;   // Э���ַ����
    arp_frame.op = htons(op);   // ��������

    return arp_frame;
}

void set_arp_message(byte *buf, int len, byte *SrcMAC, DWORD SrcIP, DWORD DesIP) {
    // ����MAC֡ͷ
    byte DesMAC[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    MACFrame_Head_t mac_frame_head = produce_mac_frame_Head(DesMAC, SrcMAC, ARP);

    // ARP���ݰ�
    ArpPacket_t arp_frame = produce_arp_frame(ARP_HARDWARE, ETH_IP, ARP_REQUEST, SrcMAC, SrcIP, DesIP);

    // ����һ��ARP����
    memset(buf, 0, len);   //ARP����
    memcpy(buf, &mac_frame_head, sizeof(MACFrame_Head_t));
    memcpy(buf + sizeof(MACFrame_Head_t), &arp_frame, sizeof(ArpPacket_t));
}

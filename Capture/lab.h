#ifndef CAPTURE_LAB_H
#define CAPTURE_LAB_H

#include "pcap.h"
#include "pcap-bpf.h"
#include "pcap-namedb.h"

const char *iptos(struct sockaddr *sock_addr);

void ifprint(pcap_if_t *d);

// 加载环境
void load_environment();

// 显示所有网卡
int get_all_dev(pcap_if_t *&all_devs);

// 拿到特定的网卡
void get_one_dev(pcap_if_t *&one_dev, pcap_if_t *&all_devs, const int &all_devs_num, int one_dev_num);

// 拿到指定网卡句柄
pcap_t *get_ad_handle(pcap_if_t *&one_dev, pcap_if_t *&all_devs);

// 发送ARP
int send_arp(pcap_t *ad_handle, DWORD DesIP, DWORD SrcIP, byte *SrcMAC);

// 捕获响应的ARP
int recv_arp(pcap_t *ad_handle, DWORD DesIP, byte *DesMAC);

// 获取本地MAC
int get_local_mac(pcap_t *ad_handle, DWORD localIP, byte *localMAC);

#endif //CAPTURE_LAB_H

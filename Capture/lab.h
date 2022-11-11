#ifndef CAPTURE_LAB_H
#define CAPTURE_LAB_H

#include "pcap.h"
#include "pcap-bpf.h"
#include "pcap-namedb.h"

const char *iptos(struct sockaddr *sock_addr);

void ifprint(pcap_if_t *d);

// ���ػ���
void load_environment();

// ��ʾ��������
int get_all_dev(pcap_if_t *&all_devs);

// �õ��ض�������
void get_one_dev(pcap_if_t *&one_dev, pcap_if_t *&all_devs, const int &all_devs_num, int one_dev_num);

// �õ�ָ���������
pcap_t *get_ad_handle(pcap_if_t *&one_dev, pcap_if_t *&all_devs);

#endif //CAPTURE_LAB_H

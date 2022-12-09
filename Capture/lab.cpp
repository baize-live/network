#include <iostream>

#include "lab.h"
#include "struct.h"

using namespace std;

#define ADDR_STR_MAX 128

// Ip תΪ �ַ���
const char *iptos(struct sockaddr *sock_addr) {
    static char address[ADDR_STR_MAX] = {0};
    int gni_error = getnameinfo(sock_addr, sizeof(struct sockaddr_storage), address, ADDR_STR_MAX, nullptr, 0,
                                NI_NUMERICHOST);
    if (gni_error != 0) {
        cerr << "getnameinfo: " << gai_strerror(gni_error) << endl;
        return "ERROR!";
    }
    return address;
}

/* Print all the available information on the given interface */
void ifprint(pcap_if_t *d) {
    pcap_addr_t *a;

    /* Name */
    printf("%s\n", d->name);

    /* Description */
    if (d->description)
        printf("\tDescription: %s\n", d->description);

    /* Loopback Address*/
    printf("\tLoopback: %s\n", (d->flags & PCAP_IF_LOOPBACK) ? "yes" : "no");

    /* IP addresses */
    for (a = d->addresses; a; a = a->next) {
        printf("\n\tAddress Family: #%d\n", a->addr->sa_family);

        switch (a->addr->sa_family) {
            case AF_INET:
                printf("\tAddress Family Name: AF_INET\n");
                break;

            case AF_INET6:
                printf("\tAddress Family Name: AF_INET6\n");
                break;

            default:
                printf("\tAddress Family Name: Unknown\n");
                break;
        }
        if (a->addr && a->addr->sa_family > 0) {
            printf("\tAddress: %s\n", iptos(a->addr));
        }
        if (a->netmask && a->netmask->sa_family > 0) {
            printf("\tNetmask: %s\n", iptos(a->netmask));
        }
        if (a->broadaddr && a->broadaddr->sa_family > 0) {
            printf("\tBroadcast Address: %s\n", iptos(a->broadaddr));
        }
        if (a->dstaddr && a->dstaddr->sa_family > 0) {
            printf("\tDestination Address: %s\n", iptos(a->dstaddr));
        }
    }
    printf("\n");
}

// ���ػ���
void load_environment() {
    WSADATA wsa_data;
    int err = WSAStartup(MAKEWORD(2, 2), &wsa_data);

    if (err != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", err);
        exit(1);
    }
}

// ��ʾ��������
int get_all_dev(pcap_if_t *&all_devs) {
    int all_devs_num = 0;               // ���ڼ��� �ӿ�
    char err_buf[PCAP_ERRBUF_SIZE];     // ������Ϣ������

    // ��ñ������豸�б�
    int ret = pcap_findalldevs_ex(PCAP_SRC_IF_STRING, nullptr, &all_devs, err_buf);
    // ������
    if (ret == -1) {
        cerr << err_buf << endl;
        exit(1);
    }
    // ��ʾ�ӿ��б�
    for (pcap_if_t *one_dev = all_devs; one_dev != nullptr; one_dev = one_dev->next) {
        ++all_devs_num;
        cout << all_devs_num << "." << one_dev->description << endl;
    }
    return all_devs_num;
}

// �õ��ض�������
void get_one_dev(pcap_if_t *&one_dev, pcap_if_t *&all_devs, const int &all_devs_num, int one_dev_num) {
    // ���޴���
    if (one_dev_num < 1 || one_dev_num > all_devs_num) {
        cerr << "������ų������� " << endl;
        // �ͷ��豸�б�
        pcap_freealldevs(all_devs);
        exit(1);
    }

    // ��λ
    one_dev = all_devs;
    while (--one_dev_num) {
        one_dev = one_dev->next;
    }
}

// �õ�ָ���������
pcap_t *get_ad_handle(pcap_if_t *&one_dev, pcap_if_t *&all_devs) {
    pcap_t *ad_handle;
    char err_buf[PCAP_ERRBUF_SIZE];     // ������Ϣ������
    if ((ad_handle = pcap_open_live(one_dev->name, 65536, PCAP_OPENFLAG_PROMISCUOUS, 1000, err_buf)) == nullptr) {
        cerr << "Unable to open the adapter. " << one_dev->name << " is not supported by Npcap" << endl;
        // �ͷ��豸�б�
        pcap_freealldevs(all_devs);
        exit(1);
    }
    /* At this point, we don't need any more the device list. �ͷ��豸�б� */
    pcap_freealldevs(all_devs);

    return ad_handle;
}

// ����ARP
int send_arp(pcap_t *ad_handle, DWORD DesIP, DWORD SrcIP, byte *SrcMAC) {
    // arp���ṹ��С��42���ֽ�
    const int arp_len = 42;
    byte send_buf[arp_len];

    if (INADDR_NONE != DesIP) {
        set_arp_message(send_buf, arp_len, SrcMAC, SrcIP, DesIP);
        // ������ͳɹ�
        pcap_sendpacket(ad_handle, send_buf, arp_len);
        return 1;
    } else {
        return -1;
    }
}

// ������Ӧ��ARP
int recv_arp(pcap_t *ad_handle, DWORD DesIP, byte *DesMAC) {
    int res;
    pcap_pkthdr *header;
    const u_char *pkt_data;
    // ��ʼ��ʱ
    DWORD start = GetTickCount();
    while (true) {
        res = pcap_next_ex(ad_handle, &header, &pkt_data);

        if (GetTickCount() - start > 5000 || res < 0) {
            return -1;
        }

        if (res == 0) {
            continue;
        }

        auto *mac_frame = (MACFrame_t *) pkt_data;
        if (htons(mac_frame->MAC_Frame_Head.FrameType) != ARP) {
            continue;
        }

        auto *arp_packet = (ArpPacket_t *) mac_frame->data;
        if (htons(arp_packet->op) != ARP_RESPONSE || arp_packet->SrcIP != DesIP) {
            continue;
        }

        memcpy(DesMAC, arp_packet->SrcMAC, 6);
        return 1;
    }
}

// ��ȡ����MAC
int get_local_mac(pcap_t *ad_handle, DWORD localIP, byte *localMAC) {
    byte fakeMAC[6] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    DWORD fakeIP = inet_addr("11.11.11.11");

    if (send_arp(ad_handle, localIP, fakeIP, fakeMAC) == 1 && recv_arp(ad_handle, localIP, localMAC) == 1) {
        return 1;
    } else {
        return -1;
    }
}

#include <iostream>

#include "pcap.h"
#include "pcap-bpf.h"
#include "pcap-namedb.h"
#include <tchar.h>

#include "parse_frame.h"

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
void get_one_dev(pcap_if_t *&one_dev, pcap_if_t *&all_devs, const int &all_devs_num) {
    int one_dev_num = 0;
    cout << "������Ҫ�������������:";
    cin >> one_dev_num;

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

    cout << "listening on ... ������Ϣ����: " << endl;
    ifprint(one_dev);

}

// �õ�ָ���������
pcap_t *get_ad_handle(pcap_if_t *&one_dev, pcap_if_t *&all_devs) {
    pcap_t *ad_handle;
    char err_buf[PCAP_ERRBUF_SIZE];     // ������Ϣ������
    if ((ad_handle = pcap_open_live(one_dev->name, 65536, 1, 1000, err_buf)) == nullptr) {
        cerr << "Unable to open the adapter. " << one_dev->name << " is not supported by Npcap" << endl;
        // �ͷ��豸�б�
        pcap_freealldevs(all_devs);
        exit(1);
    }
    /* At this point, we don't need any more the device list. �ͷ��豸�б� */
    pcap_freealldevs(all_devs);

    return ad_handle;
};

// �����ֽ���
void capture_bytes(pcap_t *&ad_handle, pcap_if_t *&all_devs) {
    // ����ı�������
    int pkt_sum = 0;
    cout << "������Ҫ����ı�������:";
    cin >> pkt_sum;

    /* Retrieve the packets */
    int res;
    pcap_pkthdr *header;
    const u_char *pkt_data;
    int pkt_num = 0;

    while ((res = pcap_next_ex(ad_handle, &header, &pkt_data)) >= 0) {
        /* Timeout elapsed */
        if (res == 0) {
            continue;
        }

        cout << "=============================" << endl;
        cout << "��" << ++pkt_num << "������ ";
        time_t local_tv_sec = header->ts.tv_sec;
        cout << "����ʱ��: " << ctime(&local_tv_sec);
        cout << "ԭʼ����: " << " ";
        print_bytes(pkt_data, header->len);
        cout << endl;

        cout << endl << "��������: " << endl;
        parse_message(pkt_data);
        cout << endl;

        if (pkt_num == pkt_sum) {
            break;
        }
    }

    if (res == -1) {
        cout << "Error reading the packets: " << pcap_geterr(ad_handle) << endl;
        exit(1);
    }

}

int main() {
    // ���ػ���
    load_environment();

    // ָ���豸�����ײ���ָ��
    pcap_if_t *all_devs;
    int all_devs_num = get_all_dev(all_devs);

    // �õ���Ҫ����������
    pcap_if_t *one_dev;
    get_one_dev(one_dev, all_devs, all_devs_num);

    // �õ�ָ���������
    pcap_t *ad_handle = get_ad_handle(one_dev, all_devs);

    // �����ֽ���
    capture_bytes(ad_handle, all_devs);

    // �ر�ָ���������
    pcap_close(ad_handle);
    WSACleanup();

    system("pause");
}

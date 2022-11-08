#include <iostream>

#include "lab.h"

using namespace std;

#define ADDR_STR_MAX 128

// Ip 转为 字符串
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

// 加载环境
void load_environment() {
    WSADATA wsa_data;
    int err = WSAStartup(MAKEWORD(2, 2), &wsa_data);

    if (err != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", err);
        exit(1);
    }
}

// 显示所有网卡
int get_all_dev(pcap_if_t *&all_devs) {
    int all_devs_num = 0;               // 用于计数 接口
    char err_buf[PCAP_ERRBUF_SIZE];     // 错误信息缓冲区

    // 获得本机的设备列表
    int ret = pcap_findalldevs_ex(PCAP_SRC_IF_STRING, nullptr, &all_devs, err_buf);
    // 错误处理
    if (ret == -1) {
        cerr << err_buf << endl;
        exit(1);
    }
    // 显示接口列表
    for (pcap_if_t *one_dev = all_devs; one_dev != nullptr; one_dev = one_dev->next) {
        ++all_devs_num;
        cout << all_devs_num << "." << one_dev->description << endl;
    }
    return all_devs_num;
}

// 拿到特定的网卡
void get_one_dev(pcap_if_t *&one_dev, pcap_if_t *&all_devs, const int &all_devs_num) {
    int one_dev_num = 0;
    cout << "请输入要监听的网卡编号:";
    cin >> one_dev_num;

    // 超限处理
    if (one_dev_num < 1 || one_dev_num > all_devs_num) {
        cerr << "网卡编号超出限制 " << endl;
        // 释放设备列表
        pcap_freealldevs(all_devs);
        exit(1);
    }

    // 复位
    one_dev = all_devs;
    while (--one_dev_num) {
        one_dev = one_dev->next;
    }

    cout << "listening on ... 网卡信息如下: " << endl;
    ifprint(one_dev);

}

// 拿到指定网卡句柄
pcap_t *get_ad_handle(pcap_if_t *&one_dev, pcap_if_t *&all_devs) {
    pcap_t *ad_handle;
    char err_buf[PCAP_ERRBUF_SIZE];     // 错误信息缓冲区
    if ((ad_handle = pcap_open_live(one_dev->name, 65536, PCAP_OPENFLAG_PROMISCUOUS, 1000, err_buf)) == nullptr) {
        cerr << "Unable to open the adapter. " << one_dev->name << " is not supported by Npcap" << endl;
        // 释放设备列表
        pcap_freealldevs(all_devs);
        exit(1);
    }
    /* At this point, we don't need any more the device list. 释放设备列表 */
    pcap_freealldevs(all_devs);

    return ad_handle;
};

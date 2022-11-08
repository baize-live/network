#include <iostream>
#include <ctime>

#include "struct.h"
#include "lab.h"

using namespace std;

// 捕获字节流
void capture_bytes(pcap_t *&ad_handle) {
    // 捕获的报文条数
    int pkt_sum = 0;
    cout << "请输入要捕获的报文条数:";
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
        cout << "第" << ++pkt_num << "条报文 ";
        time_t local_tv_sec = header->ts.tv_sec;
        cout << "捕获时间: " << ctime(&local_tv_sec);
        cout << "原始数据: " << " ";

        print_bytes((byte *) pkt_data, header->len);
        cout << endl;

        cout << endl << "解析数据: " << endl;
        parse_message((byte *) pkt_data);
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
    // 加载环境
    load_environment();

    // 指向设备链表首部的指针
    pcap_if_t *all_devs;
    int all_devs_num = get_all_dev(all_devs);

    // 拿到想要监听的网卡
    pcap_if_t *one_dev;
    get_one_dev(one_dev, all_devs, all_devs_num);

    // 拿到指定网卡句柄
    pcap_t *ad_handle = get_ad_handle(one_dev, all_devs);

    // 捕获字节流
    capture_bytes(ad_handle);

    // 关闭指定网卡句柄
    pcap_close(ad_handle);
    WSACleanup();

    system("pause");
    return 0;
}

#include <iostream>
#include <ctime>

#include "struct.h"
#include "lab.h"

using namespace std;

// �����ֽ���
void capture_bytes(pcap_t *&ad_handle) {
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

        print_bytes((byte *) pkt_data, header->len);
        cout << endl;

        cout << endl << "��������: " << endl;
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
    capture_bytes(ad_handle);

    // �ر�ָ���������
    pcap_close(ad_handle);
    WSACleanup();

    system("pause");
    return 0;
}

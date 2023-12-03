#include <fstream>

#include "protocol/RTP.h"
#include "protocol/protocol.h"
#include "utils/FileUtil.h"

const short port = 2000;
const string host = "127.0.0.1";
#define buffer_len DATA_LEN

int main() {
    RTP_Client client;
    client.client();
    if (client.connect(host, port) != 0) {
        cout << "client start failure... " << endl;
        return 0;
    }
    cout << "client start success... " << endl;

    int fileNum = 1;
    FileList fileList = Dir::entry_static("../send/");
    for (auto &file: fileList) {
        cout << fileNum++ << ". " << file->fileName() << endl;
    }
    cout << "��ѡ��һ���ļ�.." << endl;

    cin >> fileNum;
    if (fileNum > fileList.size()) {
        cout << "�ļ�ѡ��ʧ�� " << endl;
        return 0;
    }
    auto it = fileList.begin();
    for (; it != fileList.end(); ++it) {
        if (--fileNum == 0) {
            break;
        }
    }
    File file = **it;

    cout << "��ʼ���� " << file.fileName() << " ��С: " << file.fileSize() << endl;

    // �ȷ��� �ļ���С���ļ���
    Request request;
    request.set_header("fileSize", to_string(file.fileSize()));
    request.set_header("fileName", file.fileName());
    request.request(client);

    // Ȼ���� �ļ�����
    std::ifstream inFile(file.filePath(), std::ios::in | std::ios::binary);

    if (!inFile.is_open()) {
        printf("***ERROR***: Open file failure.\n");
        inFile.close();
    }

    char buf[buffer_len];
    int sum = File::fileSize(file.filePath());
    int len, now = 0;
    DWORD start = GetTickCount();
    while (now < sum) {
        len = (sum - now) < buffer_len ? (sum - now) : buffer_len;
        inFile.read(buf, len);
        client.send(buf, len);
        now += len;
    }

    DWORD end = GetTickCount();
    cout << "���ͳɹ� " << file.fileName() << endl;
    cout << "��ʱ: " << end - start << "ms" << endl;
    cout << "ƽ��������: " << sum * 1.0 / (end - start) << endl;

    inFile.close();
    if (client.close() == 0) {
        cout << "���ֳɹ�" << endl;
    }

}
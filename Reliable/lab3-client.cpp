#include <fstream>

#include "protocol/RTP.h"
#include "protocol/protocol.h"
#include "utils/FileUtil.h"

const short port = 4000;
const string host = "127.0.0.1";

int main() {
    RTP client;
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
    }
    auto it = fileList.begin();
    for (; it != fileList.end(); ++it) {
        if (--fileNum == 0) {
            break;
        }
    }
    File file = **it;

    cout << "��ʼ���� " << file.fileName() << endl;

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

    char buf[1024];
    int len = File::fileSize(file.filePath());
    int now, sum = 0;
    DWORD start = GetTickCount();
    while (sum < len) {
        now = (len - sum) < 1024 ? (len - sum) : 1024;
        inFile.read(buf, now);
        client.send(buf, now);
        sum += now;
    }
    DWORD end = GetTickCount();
    cout << "���ͳɹ� " << file.fileName() << endl;
    cout << "��ʱ: " << end - start << "ms" << endl;
    cout << "ƽ��������: " << len * 1.0 / (end - start) << endl;

    client.close();

}
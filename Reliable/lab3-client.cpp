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
    cout << "请选择一个文件.." << endl;

    cin >> fileNum;
    if (fileNum > fileList.size()) {
        cout << "文件选择失败 " << endl;
        return 0;
    }
    auto it = fileList.begin();
    for (; it != fileList.end(); ++it) {
        if (--fileNum == 0) {
            break;
        }
    }
    File file = **it;

    cout << "开始发送 " << file.fileName() << " 大小: " << file.fileSize() << endl;

    // 先发送 文件大小和文件名
    Request request;
    request.set_header("fileSize", to_string(file.fileSize()));
    request.set_header("fileName", file.fileName());
    request.request(client);

    // 然后发送 文件本体
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
    cout << "发送成功 " << file.fileName() << endl;
    cout << "用时: " << end - start << "ms" << endl;
    cout << "平均吞吐率: " << sum * 1.0 / (end - start) << endl;

    inFile.close();
    if (client.close() == 0) {
        cout << "挥手成功" << endl;
    }

}
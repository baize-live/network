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
    cout << "请选择一个文件.." << endl;

    cin >> fileNum;
    if (fileNum > fileList.size()) {
        cout << "文件选择失败 " << endl;
    }
    auto it = fileList.begin();
    for (; it != fileList.end(); ++it) {
        if (--fileNum == 0) {
            break;
        }
    }
    File file = **it;

    cout << "开始发送 " << file.fileName() << endl;

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
    cout << "发送成功 " << file.fileName() << endl;
    cout << "用时: " << end - start << "ms" << endl;
    cout << "平均吞吐率: " << len * 1.0 / (end - start) << endl;

    client.close();

}
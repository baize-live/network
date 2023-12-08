#include <fstream>

#include "protocol_4/RTP.h"

const short port = 2000;
const string host = "127.0.0.1";
#define buffer_len DATA_LEN

int main() {
    RTP_Client client;
    client.init();
    if (client.connect(host, port) != 0) {
        cout << "client start failure... " << endl;
        return 0;
    }
    cout << "client start success... " << endl;

    // 发送Buf
    char buf[buffer_len];

    cout << "请输入文件名(默认为 send 目录下文件): ";
    string fileName;
    cin >> fileName;
    std::ifstream inFile("../send/" + fileName, std::ios::in | std::ios::binary);
    if (!inFile.is_open()) {
        cout << "文件打开异常" << endl;
        exit(-1);
    }
    inFile.seekg(0, ios::end);
    int fileSize = (int) inFile.tellg();

    // 1. 发送文件大小和文件名
    int fileNameSize = (int) fileName.size();
    memcpy(buf, &fileSize, 4);
    memcpy(buf + 4, &fileNameSize, 4);
    memcpy(buf + 8, fileName.c_str(), fileNameSize);
    client.send(buf, fileNameSize + 8);

    // 2. 发送文件本体
    cout << "开始发送 " << fileName << " 大小: " << fileSize << endl;
    inFile.seekg(0, ios::beg);

    int sum = fileSize;
    int len, now = 0;
    // 计时开始
    DWORD start = GetTickCount();
    while (now < sum) {
        len = (sum - now) < buffer_len ? (sum - now) : buffer_len;
        inFile.read(buf, len);
        client.send(buf, len);
        now += len;
    }
    // 计时结束
    DWORD end = GetTickCount();
    inFile.close();
    // 统计
    cout << "发送成功 " << fileName << endl;
    cout << "用时: " << (end - start) * 1.0 / 1000 << "ms" << endl;
    cout << "平均吞吐率: " << sum * 1.0 / (end - start) << "KB/s" << endl;

    client.close();

}
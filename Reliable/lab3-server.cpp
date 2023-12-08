#include <fstream>

#include "protocol_4/RTP.h"

const short port = 2000;
#define buffer_len DATA_LEN

void handle(RTP_Client *client) {
    char buf[buffer_len];

    // 接收文件大小和文件名
    client->recv(buf, buffer_len);
    int fileSize;
    int fileNameSize;
    memcpy(&fileSize, buf, 4);
    memcpy(&fileNameSize, buf + 4, 4);
    string fileName(fileNameSize, ' ');
    copy(buf + 8, buf + 8 + fileNameSize, fileName.begin());

    // 接收文件
    cout << "开始接收 " << fileName << " 大小: " << fileSize << endl;
    std::ofstream outFile("../recv/" + fileName, std::ios::out | std::ios::binary | std::ios::app);
    int len, sum = 0;
    while (true) {
        len = client->recv(buf, buffer_len);
        if (len == -1) {
            break;
        }
        outFile.write(buf, len);
        sum += len;
        if (sum >= fileSize) {
            break;
        }
    }

    cout << "接收成功 " << fileName << endl;
    outFile.close();
}

int main() {
    RTP_Server server;
    server.server(port);
    server.listen();
    cout << "server start success... " << endl;

    int num = 0;
    while (true) {
        auto client = server.accept();
        thread t(&handle, client);
        t.detach();
        num++;
    }
}
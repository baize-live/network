#include <fstream>

#include "protocol/RTP.h"
#include "protocol/protocol.h"
#include "utils/FileUtil.h"

const short port = 2001;
const int buffer_len = 1024;

int main() {
    RTP server;
    server.server(port);
    if (server.accept() != 0) {
        cout << "server start failure... " << endl;
        return 0;
    }
    cout << "server start success... " << endl;

    char buf[buffer_len];
    int len = server.recv(buf, buffer_len);

    Request request;
    request.set_bytes(buf, len);
    string fileName = request.get_header("fileName");
    int fileSize = stoi(request.get_header("fileSize"));

    File file("../recv/" + fileName);

    cout << "开始接收 " << file.fileName() << endl;

    std::ofstream outFile(file.filePath(), std::ios::out | std::ios::binary | std::ios::app);

    int sum = 0;
    while (true) {
        len = server.recv(buf, buffer_len);
        if (len == -1) {
            break;
        }
        outFile.write(buf, len);
        sum += len;
        if (sum == fileSize) {
            break;
        }
    }
    outFile.close();
    server.close();

    cout << "接收成功 " << file.fileName() << endl;

    server.close();

}
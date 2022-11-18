#include "RTP.h"
#include "FileUtil.h"
#include<fstream>

const short port = 4000;
const string host = "127.0.0.1";

int main() {
    RTP client;
    client.client();
    if (client.connect(host, port) != 0) {
        return 0;
    }
    cout << "client 连接成功" << endl;

    string filepath = R"(C:\Users\CodeXS\Pictures\Camera Roll\壁纸.png)";
    std::ifstream inFile(filepath, std::ios::in | std::ios::binary);

    if (!inFile.is_open()) {
        printf("***ERROR***: Open file failure.\n");
        inFile.close();
    }

    char buf[1024];
    int len = File::fileSize(filepath);
    int now, sum = 0;
    while (sum < len) {
        now = (len - sum) < 1024 ? (len - sum) : 1024;
        inFile.read(buf, now);
        client.send(buf, now);
        sum += now;
    }

    Sleep(5000);
}
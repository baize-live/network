#include <thread>
#include "RTP.h"
#include <fstream>

const short port = 2001;

int main() {
    RTP server;
    server.server(port);
    if (server.accept() != 0) {
        cout << "server start failure... " << endl;
        return 0;
    }
    cout << "server start success... " << endl;




    char buf[1024];
    std::ofstream outFile("../f.png", std::ios::out | std::ios::binary | std::ios::app);
    int sum = 0;
    int len;
    while (true) {
        len = server.recv(buf, 1024);
        if (len == -1) {
            break;
        }
        outFile.write(buf, len);
        sum += len;
        if (sum == 1478193) {
            break;
        }
    }
    outFile.close();
    server.close();
    Sleep(5000);

}
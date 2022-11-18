#include <iostream>
#include <thread>
#include <synchapi.h>
#include <fstream>
#include "RTP.h"
#include "FileUtil.h"

using namespace std;
bool flag = true;

void handle_request() {
    while (flag) {
        cout << "1" << endl;
        Sleep(1000);
    }
}

int a() {
    thread handle_thread(handle_request);
    handle_thread.detach();
}

int main() {
    thread handle_thread(handle_request);
//    handle_thread.join();
    Sleep(4500);

    //    string filepath = R"(C:\Users\CodeXS\Downloads\apowersoft-online-launcher.exe)";
//    std::ifstream inFile(filepath, std::ios::in | std::ios::binary);
//
//    if (!inFile.is_open()) {
//        printf("***ERROR***: Open file failure.\n");
//        inFile.close();
//    }
//
//    std::ofstream outFile("../b.exe", std::ios::out | std::ios::binary | std::ios::app);
//
//    char buf[1024];
//
//    int len = File::fileSize(filepath);
//
//    int now, sum = 0;
//    while (sum < len) {
//        now = (len - sum) < 1024 ? (len - sum) : 1024;
//        inFile.read(buf, now);
//        outFile.write(buf, now);
//        sum += now;
//    }
}

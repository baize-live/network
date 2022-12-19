#include "struct.h"

using namespace std;

// 输出字节流 0x68 0x00
void print_bytes(byte *buf, const int len) {
    for (int i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
    }
}

// 输出IP 192.168.1.1
void print_ip(DWORD IP) {
    printf("%lu.%lu.%lu.%lu", IP & 0xFF, (IP >> 8) & 0xFF, (IP >> 16) & 0xFF, (IP >> 24) & 0xFF);
}

// 输出MAC
void print_mac(byte *MAC) {
    printf("%02X:%02X:%02X:%02X:%02X:%02X", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
}
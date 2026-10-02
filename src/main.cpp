
#include "../include/claValidation.hpp"
#include "../include/scanPorts.hpp"

#include <stdio.h> 
#include <winsock2.h>

int main(int argc, char* argv[]) {
    // Check input
    if (claValidation(argc, argv) != 0) {
        return 1;
    }
    const char* targetIP = argv[1];

    // Initialize Winsock    
    WSADATA wsaData{};
    int iResult{ WSAStartup(MAKEWORD(2,2), &wsaData) };
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    // Scan ports (0 to 1024 are the well-known ports)
    scanPorts(1, 100, targetIP);

    // Clean up and return
    WSACleanup();
    return 0;
}
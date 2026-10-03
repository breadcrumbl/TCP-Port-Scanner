
#include "claValidation.hpp"
#include "scanPorts.hpp"

#include <stdio.h> 
#include <string>
#include <winsock2.h>

int main(int argc, char* argv[]) {
    // Check input
    if (claValidation(argc, argv) != 0) {
        return 1;
    }
    const char* targetIP = argv[1];
    const int startPort = (argc == 6) ? std::stoi(argv[4]) : 0;
    const int endPort = (argc == 6) ? std::stoi(argv[5]) : 1023;

    // Initialize Winsock    
    WSADATA wsaData{};
    int iResult{ WSAStartup(MAKEWORD(2,2), &wsaData) };
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    // Scan ports (0 to 1024 are the well-known ports)
    scanPorts(startPort, endPort, targetIP);

    // Clean up and return
    WSACleanup();
    return 0;
}
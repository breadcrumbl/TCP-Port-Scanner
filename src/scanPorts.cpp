#include "../include/scanPorts.hpp"

#include <stdio.h> // For printf
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

// Scan a range of ports, printing out which are open
void scanPorts(int startPort, int endPort) {
    for (int port {startPort}; port <=endPort; ++port) {
        if(scanSinglePort(port) == 0)
            printf("Port %d is OPEN\n", port);
    }
    printf("Finished scanning ports %d to %d\n", startPort, endPort);
}

// Scan a single port, returning 0 if open, 1 if closed, and 2 if error
int scanSinglePort(int port) {
    // Turn port into string
    const char* host { "127.0.0.1" }; //TO-DO: make this CLA
    std::string portString {std::to_string(port)};

    // Get address info
    struct addrinfo *result = nullptr, *ptr = nullptr, hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    int iResult = getaddrinfo(host, portString.c_str(), &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        return 2;
    }

    // Create socket
    SOCKET ConnectSocket { INVALID_SOCKET };
    ptr=result;
    
    // Try connect
    while (ptr != nullptr) {
        // Creat socket for this address
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        // If socket not successfully created, move to next address
        if (ConnectSocket == INVALID_SOCKET) {
            //printf("Error at socket(): %ld\n", WSAGetLastError());
            ptr=ptr->ai_next;
            continue;
        }

        // Try connecting
        iResult = connect(ConnectSocket, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen));
        // If connected, close socket, free address info, and return 0 (open)
        if (iResult == 0) {
            freeaddrinfo(result);
            closesocket(ConnectSocket);
            return 0; 
        }
        // If connection failed, close socket and move to next address
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
        ptr=ptr->ai_next;
    }

    // Free address info and return 1 (closed) if all connection attempts failed
    freeaddrinfo(result);
    return 1; 
}


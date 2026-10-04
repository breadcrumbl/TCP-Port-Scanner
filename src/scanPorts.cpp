#include "scanPorts.hpp"

#include <stdio.h> 
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

// Scan a range of ports, printing out which are open
void scanPorts(int startPort, int endPort, const char* targetIP) {
    for (int port {startPort}; port <=endPort; ++port) {
        const PortStatus result = scanSinglePort(port, targetIP);
        switch (result) {
            case PortStatus::open:
                printf("Port %d: OPEN\n", port);
                break;
            case PortStatus::closed:
                printf("Port %d: CLOSED\n", port);
                break;
            case PortStatus::filtered:
                printf("Port %d: may be FILTERED or unresponsive\n", port);
                break;
            case PortStatus::error:
                printf("Port %d: ERROR\n", port);
                break;   
        }
    }
    printf("Finished scanning ports %d to %d\n", startPort, endPort);
}

// Scan a single port and return its status
PortStatus scanSinglePort(int port, const char* targetIP) {
    // Turn port into string
    std::string portString {std::to_string(port)};

    // Get address info
    struct addrinfo *result = nullptr, hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    int iResult = getaddrinfo(targetIP, portString.c_str(), &hints, &result);
    if (iResult != 0) {
        printf("Error at getaddrinfo() for port %d: %i\n", port, iResult);
        return PortStatus::error;
    }

    // Create socket
    SOCKET ConnectSocket { INVALID_SOCKET };
    
    // Try connect
    bool connectionRefused { false }, connectionTimedOut { false };
    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        // Creat socket for this address
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        // If socket not successfully created, move to next address
        if (ConnectSocket == INVALID_SOCKET) {
            printf("Error at socket() for port %d: %i\n", port, WSAGetLastError());
            continue;
        }

        // Try connecting
        iResult = connect(ConnectSocket, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen));
        // If connected, close socket, free address info, and return
        if (iResult == 0) {
            freeaddrinfo(result);
            closesocket(ConnectSocket);
            return PortStatus::open;
        }

        // If connection failed, determine why and close socket
        iResult = WSAGetLastError();
        closesocket(ConnectSocket);
        // If connection refused, port is closed, if timed out, port is filtered. Otherwise, return error
        if (iResult == WSAECONNREFUSED) {
            connectionRefused = true;
        }
        else if (iResult == WSAETIMEDOUT) {
            connectionTimedOut = true;
        } 
        else {
            printf("Error at connect() for port %d: %i\n", port, iResult);
        }
    }

    // Free address info and return if all connection attempts failed
    freeaddrinfo(result);
    if (connectionRefused) {
        return PortStatus::closed;
    }
    else if (connectionTimedOut) {
        return PortStatus::filtered;
    }
    return PortStatus::error;
}


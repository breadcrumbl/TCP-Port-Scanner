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
                printf("Port %i: CLOSED\n", port);
                break;
            case PortStatus::filtered:
                printf("Port %i: may be FILTERED or unresponsive\n", port);
                break;
            case PortStatus::error:
                printf("Port %i: ERROR\n", port);
                break;   
        }
    }
    printf("Finished scanning ports %i to %i\n", startPort, endPort);
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
        printf("Error at getaddrinfo() for port %i: %i\n", port, iResult);
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
            printf("Error at socket() for port %i: %i\n", port, WSAGetLastError());
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
            printf("Error at connect() for port %i: %i\n", port, iResult);
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

// Creat a single non-blocking socket
namespace NonBlocking {
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
        printf("Error at getaddrinfo() for port %i: %i\n", port, iResult);
        return PortStatus::error;
    }

    // Create socket
    SOCKET ConnectSocket { INVALID_SOCKET };
    
    // Try connect
    bool connectionRefused { false }, connectionTimedOut { false };
    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        // Creat socket for this address; if socket not successfully created, move to next address
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("Error at socket() for port %i: %i\n", port, WSAGetLastError());
            continue;
        }

        // Enable non-blocking mode
        u_long nonBlocking { 1 };
        // Configure connectSocket according to the FIONBIO command, using the value in nonBlocking
        iResult = ioctlsocket(ConnectSocket, FIONBIO, &nonBlocking);
        if (iResult == SOCKET_ERROR) {
            printf("Error at ioctlsocket() for port %i: %i\n", port, WSAGetLastError());
            closesocket(ConnectSocket);
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

        // If a connection is not still in progress (WSAEWOULDBLOCK or WSAEINPROGRESS), report why and close socket 
        // Why: connection was refused (port closed), timed out (port filtered), or another error
        iResult = WSAGetLastError();
        if (iResult != WSAEWOULDBLOCK && iResult != WSAEINPROGRESS) {
            if (iResult == WSAECONNREFUSED) {
                connectionRefused = true;
            }
            else if (iResult == WSAETIMEDOUT) {
                connectionTimedOut = true;
            } 
            else {
                printf("Error at connect() for port %i: %i\n", port, iResult);
            }
            closesocket(ConnectSocket);
            continue;
        }

        // Tell when connection is established
        // Create and clear a container to put socket then add socket
        // Called writeSet because a socket becoming writable is a way Winsock a non-blocking connect() attempt has completed
        FD_SET writeSet{}; 
        FD_ZERO(&writeSet);
        FD_SET(ConnectSocket, &writeSet);

        // Create a timeout (in microseconds)
        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;

        // Call select
        iResult = select(
            0,              // Number of sockets
            nullptr,        // Ptr to read set (not interested to set null)
            &writeSet,      // Ptr to write set
            nullptr,        // Ptr to exception set
            &timeout        // Timeout (wait max timeout.tv_usec)
        );
        // select() failed
        if (iResult == SOCKET_ERROR) {
            printf("Error at select() for port %i: %i", port, iResult);
            closesocket(ConnectSocket);
            continue;
        }
        // Timeout expired
        else if (iResult == 0) {
            connectionTimedOut = true;
            closesocket(ConnectSocket);
            continue;
        }

        // Socket becomes writable - connection could have succeeded or failed
        else if (iResult > 0) {
            // Ask what error status the socket currently has
            int optionLength = sizeof(iResult);     // Size we're going to store error in
            if(getsockopt(
                ConnectSocket,                      // Which socket
                SOL_SOCKET,                         // Asking about an option beloning to the socket itself
                SO_ERROR,                           // Asking for sockets pending error
                reinterpret_cast<char*>(&iResult),  // Asking getsockopt to write the error into iResult
                &optionLength                       // Address storing size of place to write error tp
            ) == SOCKET_ERROR) {
                printf("Error at getsockopt() for port %i: %i", port, WSAGetLastError());
                closesocket(ConnectSocket);
                continue;
            }
            closesocket(ConnectSocket);
            // If open
            if (iResult == 0) {
                freeaddrinfo(result);
                return PortStatus::open;
            }
            // If timed out
            if (iResult == WSAETIMEDOUT) {
                connectionTimedOut = true;
            }
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
};


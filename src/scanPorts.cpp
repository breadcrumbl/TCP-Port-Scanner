#include "scanPorts.hpp"

#include <chrono> // For debugging
#include <thread>
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
                printf("Port %i: may be FILTERED or UNRESPONSIVE\n", port);
                break;
            case PortStatus::error:
                printf("Port %i: ERROR\n", port);
                break;   
        }
    }
    printf("Finished scanning ports %i to %i\n", startPort, endPort);
}


PortStatus scanSinglePort(int port, const char* targetIP) {
    // Turn port into string
    std::string portString {std::to_string(port)};

    // Get address info
    /*
    * SYNTAX 
    * typedef struct addrinfo {
    *    int             ai_flags;
    *    int             ai_family;
    *    int             ai_socktype;
    *    int             ai_protocol;
    *    size_t          ai_addrlen;
    *    char            *ai_canonname;
    *    struct sockaddr *ai_addr;
    *    struct addrinfo *ai_next;
    *    } ADDRINFOA, *PADDRINFOA;
    */
    struct addrinfo *result = nullptr, hints{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    /*
    * SYNTAX
    * INT WSAAPI getaddrinfo(
    *   [in, optional] PCSTR           pNodeName,     // A pointer to a NULL-terminated ANSI string that contains a host (node) name or a numeric host address string
    *   [in, optional] PCSTR           pServiceName,  // A pointer to a NULL-terminated ANSI string that contains either a service name or port number represented as a string.
    *   [in, optional] const ADDRINFOA *pHints,       // A pointer to an addrinfo structure that provides hints about the type of socket the caller supports.
    *   [out]          PADDRINFOA      *ppResult      // A pointer to a linked list of one or more addrinfo structures that contains response information about the host.
    * );
    * RETURN VALUE
    * Success returns zero. Failure returns a nonzero Windows Sockets error code, as found in the Windows Sockets Error Codes.
    */
    const int addressResult { getaddrinfo(targetIP, portString.c_str(), &hints, &result) };
    if (addressResult != 0) {
        printf("Error at getaddrinfo() for port %i: %i\n", port, addressResult);
        return PortStatus::error;
    }

    // Create socket
    SOCKET ConnectSocket{};
    
    // Try connect
    bool connectionRefused { false }, connectionTimedOut { false };
    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {

        // Create socket for this address; if socket not successfully created, move to next address
        /*
        * SOCKET WSAAPI socket(
        *   [in] int af,           // The address family specification (e.g. AF_UNSPEC for unspecified, AF_INET for IPv4)
        *   [in] int type,         // The type specification for the new socket (e.g. SOCK_STREAM for TCP, SOCK_DGRAM for UDP)
        *   [in] int protocol      // The protocol to be used (e.g. IPPROTO_TCP for TCP, IPPROTO_UDP for UDP)
        * );
        * RETURN VALUE
        * If no error occurs, socket returns a descriptor referencing the new socket. 
        * Otherwise, a value of INVALID_SOCKET is returned, and a specific error code can be retrieved by calling WSAGetLastError
        */
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("Error at socket() for port %i: %i\n", port, WSAGetLastError());
            continue;
        }

        // Enable non-blocking mode:  configure connectSocket according to the FIONBIO command, using the value in nonBlocking
        /*
        * SYNTAX
        * int ioctlsocket(
        *   [in]      SOCKET s,        // A descriptor identifying a socket.
        *   [in]      long   cmd,      // A command to perform on the socket s (e.g. FIONBIO enables or disables non-blocking mode)
        *   [in, out] u_long *argp     // A pointer to a parameter for cmd (e.g. for FIONBIO 1 = non-blocking)
        * );
        * RETURN VALUE
        * Upon successful completion, the ioctlsocket returns zero. 
        * Otherwise, a value of SOCKET_ERROR is returned, and a specific error code can be retrieved by calling WSAGetLastError.
        */
        u_long nonBlocking { 1 };
        const int socketResult { ioctlsocket(ConnectSocket, FIONBIO, &nonBlocking) };
        if (socketResult == SOCKET_ERROR) {
            printf("Error at ioctlsocket() for port %i: %i\n", port, WSAGetLastError());
            closesocket(ConnectSocket);
            continue;
        }

        // Try connecting
        /*
        * SYNTAX
        * int WSAAPI connect(
        *    [in] SOCKET         s,         // A descriptor identifying an unconnected socket.
        *    [in] const sockaddr *name,     // A pointer to the sockaddr structure to which the connection should be established
        *    [in] int            namelen    // The length, in bytes, of the sockaddr structure pointed to by the name parameter
        * );
        * RETURN VALUE
        * If no error occurs, connect returns zero. 
        * Otherwise, it returns SOCKET_ERROR, and a specific error code can be retrieved by calling WSAGetLastError.
        * Blocking: return value indicates success or failure of the connection attempt
        * Non-blocking: the connection attempt cannot be completed immediately so connect will return SOCKET_ERROR, and WSAGetLastError will return WSAEWOULDBLOCK.
        * Error codes indicating connection failed: 
        *   WSAECONNREFUSED (connection rejected) 
        *   WSAENETUNREACH (network cannot be reached from this host) 
        *   WSAETIMEDOUT (attemp timed out without establishing a connection)
        */
        const int connectResult { connect(ConnectSocket, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen)) };
        // Connection succeeds immediately 
        if (connectResult == 0) {
            freeaddrinfo(result);
            closesocket(ConnectSocket);
            return PortStatus::open;
        }
        // If a connection is not still in progress (WSAEWOULDBLOCK or WSAEINPROGRESS), report why and close socket 
        // Why: connection was refused (port closed), timed out (port filtered), or another error
        const int connectError { WSAGetLastError() };
        if (connectError != WSAEWOULDBLOCK && connectError != WSAEINPROGRESS) {
            if (connectError == WSAECONNREFUSED) {
                connectionRefused = true;
            }
            else if (connectError == WSAETIMEDOUT) {
                connectionTimedOut = true;
            } 
            else {
                printf("Error at connect() for port %i: %i\n", port, connectError);
            }
            closesocket(ConnectSocket);
            continue;
        }

        // Tell when connection is established
        // Create and clear a container to put socket then add socket
        // Called writeSet because a socket becoming writable is a way Winsock a non-blocking connect() attempt has completed

        // Declares sets
        fd_set writeSet{};
        fd_set exceptSet{};
        // Initilaises set to empty set
        FD_ZERO(&writeSet);
        FD_ZERO(&exceptSet);
        // Adds the socket to set
        FD_SET(ConnectSocket, &writeSet);
        FD_SET(ConnectSocket, &exceptSet);

        // Create a timeout (in microseconds)
        /*
        * typedef struct timeval {
        *    long tv_sec;                       // Time interval, seconds
        *    long tv_usec;                      // Time interval, microseconds - used in combination with the tv_sec member to represent time interval values that are not a multiple of seconds
        * } TIMEVAL, *PTIMEVAL, *LPTIMEVAL;
        */
        timeval timeout{};
        timeout.tv_sec = 2; // NOTE: select() for closed non-blocking sockets takes ~2015ms (odd, should be immediate) but that is why this time is chosen
        timeout.tv_usec = 500000;

        // Call select
        /*
        * SYNTAX
        * int WSAAPI select(
        *   [in]      int           nfds,         // Ignored. The nfds parameter is included only for compatibility with Berkeley sockets.
        *   [in, out] fd_set        *readfds,     // An optional pointer to a set of sockets to be checked for readability.
        *   [in, out] fd_set        *writefds,    // An optional pointer to a set of sockets to be checked for writability.
        *   [in, out] fd_set        *exceptfds,   // An optional pointer to a set of sockets to be checked for errors.
        *   [in]      const timeval *timeout      // The maximum time for select to wait, provided in the form of a TIMEVAL structure. Set the timeout parameter to null for blocking operations.
        * );
        * RETURN VALUE
        * The select function returns the total number of socket handles that are ready and contained in the fd_set structures, 
        * zero if the time limit expired, or SOCKET_ERROR if an error occurred. 
        * If the return value is SOCKET_ERROR, WSAGetLastError can be used to retrieve a specific error code.
        */
        const auto start = std::chrono::steady_clock::now();
        const int selectResult {  select(
            0,              // Number of sockets
            nullptr,        // Ptr to read set (not interested to set null)
            &writeSet,      // Ptr to write set
            &exceptSet,     // Ptr to exception set
            &timeout        // Timeout (wait max timeout.tv_usec)
        )};
        const auto end = std::chrono::steady_clock::now();

        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                end - start
            ).count();

        printf("select result: %d, elapsed: %lld ms\n",
            selectResult,
            static_cast<long long>(elapsed));

        // select() failed
        if (selectResult == SOCKET_ERROR) {
            printf("Error at select() for port %i: %i\n", port, WSAGetLastError());
            closesocket(ConnectSocket);
            continue;
        }
        // Timeout expired
        else if (selectResult == 0) {
            printf("Timeout at select() for port %i\n", port);
            connectionTimedOut = true;
            closesocket(ConnectSocket);
            continue;
        }
        // Connection attempt is completed (socket becomes writable); check whether succeeded or failed
        else if (selectResult > 0) {
            // Ask what error status the socket currently has
            /*
            * int getsockopt(
            *    [in]      SOCKET s,            // A descriptor identifying a socket
            *    [in]      int    level,        // The level at which the option is defined
            *    [in]      int    optname,      // The socket option for which the value is to be retrieved
            *    [out]     char   *optval,      // A pointer to the buffer in which the value for the requested option is to be returned
            *    [in, out] int    *optlen       // A pointer to the size, in bytes, of the optval buffer
            * );
            * RETURN VALUE
            * If no error occurs, getsockopt returns zero. 
            * Otherwise, a value of SOCKET_ERROR is returned, and a specific error code can be retrieved by calling WSAGetLastError.
            */
            int connectionError { 0 };                      // Store error here 
            int optionLength { sizeof(connectionError) };   // Size we're going to store error in
            const int optionResult { getsockopt(
                ConnectSocket,                              // Which socket
                SOL_SOCKET,                                 // Asking about an option beloning to the socket itself
                SO_ERROR,                                   // Asking for sockets pending error
                reinterpret_cast<char*>(&connectionError),  // Asking getsockopt to write the error into connectionError
                &optionLength                               // Address storing size of place to write error tp
            )};
            // If getsockopt() failed
            if(optionResult == SOCKET_ERROR) {
                printf("Error at getsockopt() for port %i: %i", port, WSAGetLastError());
                closesocket(ConnectSocket);
                continue;
            }
            // Open or timed out
            closesocket(ConnectSocket);
            if (connectionError == 0) {
                freeaddrinfo(result);
                return PortStatus::open;
            }
            else if (connectionError == WSAECONNREFUSED) {
                connectionRefused = true;
            }
            else if (connectionError == WSAETIMEDOUT) {
                connectionTimedOut = true;
            }
            else {
                printf("Connection failed on port %i: %i\n", port, connectionError);
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


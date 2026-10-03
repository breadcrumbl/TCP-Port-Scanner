# TCP Port Scanner

## What does it do?

A beginner C++ TCP port scanner for Windows, built using the Winsock API. The program attempts to establish TCP connections to a specified range of ports and reports which ports accept a connection.

## How does it work?
## Outline
- Starts program
- Parses command-line arguments
- Initiates Winsock 2.2
- For each port
    - Gets address information
    - Creates a TCP socket
    - Tries to connect to socket, if successful prints: "Port <N> is OPEN"
    - Closes socket
- Cleans up Windsock
- Ends program

## TCP Specifics
- Family is AF_UNSPEC (unspecified i.e. program looks for both IPv4 and IPv6 addresses)
- Socket type is SOCK_STREAM
- Protocol is IPPROTO_TCP

### Project Structure
Project Structure  
.  
├── include/  
│   └── claValidation.hpp  
|   └── scanPorts.hpp  
│  
├── src/  
│   ├── main.cpp  
│   └── claValidation.cpp  
│   └── scanPorts.cpp  
│  
└── README.md  

## Building
The program has been developed using g++ and the Windows Winsock library. 
From the project directoy, compile with:

g++ -std=c++20 -Wall -Wextra src/main.cpp src/scanPorts.cpp src/claValidation.cpp -Iinclude -o PortScanner.exe -lws2_32

## Running
### Port Range Unspecified
./PortScanner -ip <target IP address>  
./PortScanner -ip 127.0.0.1  
Default port range: 0 to 1023  
### Port Range Specified
./PortScanner.exe -ip <target IP address> -p <start port> <end port>  
./PortScanner -ip 127.0.0.1 -p 1000 5000  
Max port range: 0 to 65,534  

## Return Values
- 0: TCP connection succeeded / Port open
- 1: All connection attempts failed
- 2: Error resolving the address

## Limitations
- Currently the program treats a failed connection as a closed port when there are other reasons a connection might fail (firewall, connection timeout)

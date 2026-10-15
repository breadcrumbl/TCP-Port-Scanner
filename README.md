# TCP Port Scanner

## What does it do?

A beginner C++ TCP port scanner for Windows, built using the Winsock API. The program attempts to establish TCP connections to a specified range of ports and reports which ports accept a connection.

## How does it work?
### Outline
- Starts program
- Parses command-line arguments
- Initiates Winsock 2.2
- For each port
    - Gets address information
    - Creates a TCP socket
    - Tries to connect to socket and prints the result
    - Closes socket
- Cleans up Windsock
- Ends program

### TCP Specifics
- Family is AF_UNSPEC (unspecified i.e. program looks for both IPv4 and IPv6 addresses)
- Socket type is SOCK_STREAM
- Protocol is IPPROTO_TCP

### Project Structure
Project Structure  
```
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
```

## Building
The program has been developed using g++ and the Windows Winsock library. 
From the project directoy, compile with:

```
g++ -std=c++20 -Wall -Wextra src/main.cpp src/scanPorts.cpp src/claValidation.cpp -Iinclude -o PortScanner.exe -lws2_32
```

## Running  
### Single Port Specified
./PortScanner -ip \<target IP address\> -p \<port\>  
./PortScanner -ip 127.0.0.1 -p 80
### Port Range Specified
./PortScanner -ip \<target IP address\> -p \<start port\> \<end port\>  
./PortScanner -ip 127.0.0.1 -p 1 5  
Max port range: 0 to 65,534 
### Ports Unspecified
./PortScanner -ip \<target IP address\>  
./PortScanner -ip 127.0.0.1  
Default port range: 1 to 1024 

## Non-Blocking Sockets
The program was updated to use non-blocking sockets instead of blocking.
### Blocking Sockets
While the program attempts to connect to a blocking socket, the rest of the thread does not execute. If the socket is taking a while to connect, this can hold the whole program up.
```
     connect()
        |
        :   
Wait for response (~2 secs)
        :
        |
  conect() returns
        |
  program continues
```
### Non-Blocking Sockets
With a non-blocking socket, connect() returns immediately, telling the program the connection has succeeded or is still in progress. If the connection is still in progress, select() is called on the socket, which waits until the connection attempt completes (by checking when the socket becomes writable to) and returns open status is completes successfully or closed status if not, or if the connection attempt times out based on a chosen time limit (500 ms in this program). The advantage of this is that a single port taking a long time will not hold up the program anymore.

```
           connect()    
              |
    .---------.---------.  
    |                   | 
succeeds            in progress    
    |                   |
   OPEN              select()    
                        |
           .------------.------------.    
           |                         |   
       completes                  500 ms passes    
           |                         |
       check SO_ERROR              TIMEOUT    
           |                         |
      OPEN / CLOSED         FILTERED / UNRESPONSIVE
```
## Multi-Threading
The program was updated to use multiple threads to increase the speed. How it works:\
In the scanPorts() function:
- Decides how many threads to use by calculating the minimum of number of ports, max threads available on computer (which defaults to 4 if no result returned), and a default maximum of 8
- Creates a vector to contain these threads, a vector to contain each port (number and status), and an atomic counter shared by the threads
- Starts each of the threads using emplace_back, which passes a lambda function to each thread which
    - Fetches the next port number using the atomic counter
    - Scans that port using scanSinglePort(), which returns a port object containing the port number and status
    - Adds the port to the vector of ports
## Displaying Results
The results are printed to the console by the printPortArray() function.

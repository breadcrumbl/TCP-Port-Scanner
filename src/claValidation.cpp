#include "claValidation.hpp"

#include <stdio.h>
#include <string>

bool isCLAValid(int argc, char* argv[]) {
    // Extract program name from argv[0] for usage message
    const char* programName = strrchr(argv[0], '\\');
    if (programName != nullptr) {
        ++programName;  // Skip the '\'
    } 
    else {
        programName = argv[0];
    }

    // If no ports provided
    if (argc == 3 && std::string{argv[1]} == "-ip") {
        return true;
    }

    // If ports provided
    if ((argc == 5 || argc == 6) && std::string{argv[1]} == "-ip" && std::string{argv[3]} == "-p") {
        int startPort{}, endPort{};
        try {
            startPort = std::stoi(argv[4]); 
            if (argc == 6) {
                endPort = std::stoi(argv[5]);
            }
        }
        catch (const std::exception&) {
            printf("Ports must be valid integers.");
            return false;
        }
        
        if (startPort < 0 || startPort > 65535 || endPort < 0 || endPort > 65535) {
            printf("Ports must be valid integers between 0 and 65,535.");
            return false;
        }
        if (argc == 6 && startPort > endPort) {
            printf("Start port must be less than or equal to end port.");
            return false;
        }
        return true;
    }
    printf("Usage:\n./%s -ip <target IP address>\nor\n./%s -ip <target IP address> -p <port>\nor\n./%s -ip <target IP address> -p <start port> <end port> \n", programName, programName, programName);
    return false;
}
